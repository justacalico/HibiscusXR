#!/usr/bin/env python3
# Patch the stock Adreno blob so eglCreateContext accepts
# EGL_CONTEXT_OPENGL_ROBUST_ACCESS (0x30B2, EGL 1.5 core).
#
# The V@378 driver validates context attribs in a key dispatcher inside
# libGLESv2_adreno.so. It knows the EXT variant (0x30BF) and the KHR flags
# attrib, but 0x30B2 falls through to the unknown-key path and the whole
# eglCreateContext fails with EGL_BAD_ATTRIBUTE. The driver advertises EGL 1.5,
# so callers that follow the spec send the core token and die here - wgpu's GL
# backend does exactly this, which is what killed alvr_initialize_opengl
# (enumerate_adapters() -> empty -> remove(0) panic).
#
# The patch rewrites the head of the unknown-key path: if the key is 0x30B2 it
# branches into the existing 0x30BF handler (identical semantics - sets the
# robustness bit the driver already understands). Anything else returns
# failure as before, minus the "invalid attrib" debug log it used to emit.
# Xref audit: the only branches into the region are the three dispatch
# compares targeting the block head itself, so mid-block rewrites are safe.
#
#   before (0x15fcf8):  orr w0,wzr,#1 / bl logchk / cbz / adrp...
#   after:              mov w14,#0x30b2 ; cmp w21,w14 ; b.eq 0x15fb4c
#                       mov w0,wzr      ; b  0x15ff3c ; nop pad
#
# Usage: 405_patch_gles_robust.py <in.so> <out.so>
# Exit 0 and prints PATCH OK only when the exact expected bytes were found and
# replaced - any other blob revision fails loudly instead of guessing. An
# already-patched input is copied through unchanged.
import sys

SITE = 0x15FCF8          # unknown-key path entry
HANDLER = 0x15FB4C       # 0x30BF robust-access handler
EPILOGUE = 0x15FF3C      # function epilogue
KEEP_BR = 0x15FD28       # retained `b 0x15ff34` right after the log block

# full 48 bytes being replaced, plus anchors at every jump target
ORIG_BLOCK = bytes.fromhex(
    "e0030032"   # orr  w0, wzr, #1
    "283d0094"   # bl   0x16f19c
    "e01100b4"   # cbz  x0, 0x15ff3c
    "612a0090"   # adrp x1, 0x6ab000
    "622a0090"   # adrp x2, 0x6ab000
    "652a0090"   # adrp x5, 0x6ab000
    "21e40691"   # add  x1, x1, #0x1b9
    "420c0891"   # add  x2, x2, #0x203
    "a5640891"   # add  x5, x5, #0x219
    "430c8052"   # mov  w3, #0x62
    "e4031e32"   # orr  w4, wzr, #4
    "e603152a"   # mov  w6, w21
)
ORIG_HANDLER = bytes.fromhex("895e40b9")   # ldr w9, [x20, #0x5c]
ORIG_EPILOGUE = bytes.fromhex("fd7b42a9")  # ldp x29, x30, [sp, #0x20]
ORIG_KEEP_BR = bytes.fromhex("83000014")   # b 0x15ff34
PATCHED_HEAD = bytes.fromhex("4e168652")   # mov w14, #0x30b2


def w32(v):
    return v.to_bytes(4, "little")


def vaddr_to_off(data, vaddr):
    e_phoff = int.from_bytes(data[0x20:0x28], "little")
    e_phentsize = int.from_bytes(data[0x36:0x38], "little")
    e_phnum = int.from_bytes(data[0x38:0x3A], "little")
    for i in range(e_phnum):
        p = e_phoff + i * e_phentsize
        p_type = int.from_bytes(data[p:p + 4], "little")
        if p_type != 1:  # PT_LOAD
            continue
        p_offset = int.from_bytes(data[p + 8:p + 16], "little")
        p_vaddr = int.from_bytes(data[p + 16:p + 24], "little")
        p_filesz = int.from_bytes(data[p + 32:p + 40], "little")
        if p_vaddr <= vaddr < p_vaddr + p_filesz:
            return vaddr - p_vaddr + p_offset
    raise SystemExit("vaddr 0x%x not in any PT_LOAD" % vaddr)


def expect(data, off, want, what):
    got = bytes(data[off:off + len(want)])
    if got != want:
        raise SystemExit(
            "%s does not match this blob revision "
            "(found %s) - patch needs re-deriving" % (what, got.hex()))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: %s <in.so> <out.so>" % sys.argv[0])
    data = bytearray(open(sys.argv[1], "rb").read())
    if len(data) < 0x40 or data[:4] != b"\x7fELF" \
            or data[4] != 2 or data[5] != 1 or data[18:20] != b"\xb7\x00":
        raise SystemExit("not an aarch64 ELF64 shared object")

    off = vaddr_to_off(data, SITE)

    if bytes(data[off:off + 4]) == PATCHED_HEAD:
        open(sys.argv[2], "wb").write(data)
        print("input already patched, copied through")
        print("PATCH OK")
        return

    expect(data, off, ORIG_BLOCK, "unknown-key path at 0x%x" % SITE)
    expect(data, vaddr_to_off(data, HANDLER), ORIG_HANDLER,
           "robust handler at 0x%x" % HANDLER)
    expect(data, vaddr_to_off(data, EPILOGUE), ORIG_EPILOGUE,
           "epilogue at 0x%x" % EPILOGUE)
    expect(data, vaddr_to_off(data, KEEP_BR), ORIG_KEEP_BR,
           "branch at 0x%x" % KEEP_BR)

    def b_eq(target, at):
        imm = (target - at) // 4
        assert -(1 << 18) <= imm < (1 << 18), "b.eq out of range"
        return w32(0x54000000 | ((imm & 0x7FFFF) << 5) | 0)

    def b(target, at):
        imm = (target - at) // 4
        assert -(1 << 25) <= imm < (1 << 25), "b out of range"
        return w32(0x14000000 | (imm & 0x3FFFFFF))

    body = b"".join([
        w32(0x5286164E),            # mov w14, #0x30b2
        w32(0x6B0E02BF),            # cmp w21, w14
        b_eq(HANDLER, SITE + 8),    # b.eq -> robust handler
        w32(0x2A1F03E0),            # mov w0, wzr
        b(EPILOGUE, SITE + 16),     # b -> epilogue (ret failure)
    ])
    body += w32(0xD503201F) * 7     # nop pad over the old log block
    assert len(body) == len(ORIG_BLOCK)
    data[off:off + len(body)] = body

    open(sys.argv[2], "wb").write(data)
    print("PATCH OK")


if __name__ == "__main__":
    main()

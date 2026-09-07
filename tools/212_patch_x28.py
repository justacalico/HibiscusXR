#!/usr/bin/env python3
"""Preserve x28 across the SetClientStatusCallback call in pvr_EnterVrMode.

pvr_EnterVrMode does:

    5ff54:  bl   0xf6ec0        ; SetClientStatusCallback
    5ff58:  ldr  x0, [x28, #8]  ; x28 comes back 0 -> fault addr 0x8

x28 holds the hmdInfo struct allocated at 5fabc and is used correctly right up to
the hmdInfo logging, so it is genuinely destroyed across that call. The call is a
direct intra-library branch, so LD_PRELOAD cannot interpose it.

Instead redirect the bl into dead space between BeginDirectRendering and
EndDirectRendering (0xd0078, 4084 zero bytes, no symbol covers it) and save and
restore x28 there:

    stp x28, x30, [sp, #-16]!
    bl  <SetClientStatusCallback>
    ldp x28, x30, [sp], #16
    ret

This is a targeted workaround for a vendor bug, not a fix for whatever destroys
the register - it is deliberately narrow so it can be reverted on its own.
"""
import struct, sys, shutil

SRC = sys.argv[1] if len(sys.argv) > 1 else r"/mnt/f/PN2Lineage/notes/vrshell_lib/libPvr_UnitySDK.so"
DST = sys.argv[2] if len(sys.argv) > 2 else r"/mnt/f/PN2Lineage/notes/vrshell_lib/libPvr_UnitySDK.patched.so"

CALL_SITE = 0x5ff54     # the bl we redirect
TARGET    = 0xf6ec0     # SetClientStatusCallback
CAVE      = 0xd0080     # inside the 4084-byte gap, 16-byte aligned

shutil.copyfile(SRC, DST)
data = bytearray(open(DST, "rb").read())

def bl(frm, to):
    off = (to - frm) >> 2
    assert -(1 << 25) <= off < (1 << 25), "bl out of range"
    return struct.pack("<I", 0x94000000 | (off & 0x03FFFFFF))

def b(frm, to):
    off = (to - frm) >> 2
    return struct.pack("<I", 0x14000000 | (off & 0x03FFFFFF))

# .text is mapped at its file offset in this object (vaddr == offset), verified
# against the section header, so no translation is needed.
orig = struct.unpack_from("<I", data, CALL_SITE)[0]
expect = struct.unpack("<I", bl(CALL_SITE, TARGET))[0]
print(f"call site {CALL_SITE:#x}: {orig:#010x} (expected {expect:#010x})")
if orig != expect:
    print("FAIL: call site does not contain the expected bl, refusing to patch")
    sys.exit(1)

# make sure the cave really is empty before writing into it
cave_bytes = bytes(data[CAVE:CAVE + 32])
if any(cave_bytes):
    print(f"FAIL: cave at {CAVE:#x} is not zero-filled")
    sys.exit(1)

tramp = bytearray()
tramp += struct.pack("<I", 0xA9BF7BFC)      # stp x28, x30, [sp, #-16]!
tramp += bl(CAVE + 4, TARGET)               # bl SetClientStatusCallback
tramp += struct.pack("<I", 0xA8C17BFC)      # ldp x28, x30, [sp], #16
tramp += struct.pack("<I", 0xD65F03C0)      # ret
data[CAVE:CAVE + len(tramp)] = tramp

# point the original call at the trampoline
data[CALL_SITE:CALL_SITE + 4] = bl(CALL_SITE, CAVE)

open(DST, "wb").write(data)
print(f"trampoline written at {CAVE:#x} ({len(tramp)} bytes)")
print(f"call site now: {struct.unpack_from('<I', data, CALL_SITE)[0]:#010x} -> {CAVE:#x}")
print(f"wrote {DST}")

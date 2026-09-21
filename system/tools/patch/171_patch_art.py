#!/usr/bin/env python3
"""Patch art_quick_generic_jni_trampoline to restore sp from x29 instead of x28.

The trampoline sets BOTH registers to the same frame base:

    +68   mov x28, sp
    +72   mov x29, sp
    ...
    +144  blr x16            <- the native method
    +172  mov sp, x28        <- x28 comes back 0, so sp becomes 0
    +176  ldp d0, d1, [sp,#0x10]   <- faults at 0x10

In the tombstone x20 and x28 are zero while x19, x21-x26 and x29 are all intact.
Callee-saved registers are restored from the callee's frame, so two of them coming
back zero while their neighbours survive points at something writing zeros over a
saved-register area - not at a register-allocation bug (a real ABI violation in
Unity would break Unity everywhere on Q, which it does not).

x29 holds the identical value and survives, so restoring from it steps around the
damage. This is a probe as much as a fix: if VR then works the corruption is
narrow; if it fails elsewhere it is broad and needs a different answer.

    mov sp, x28  =  0x9100039F
    mov sp, x29  =  0x910003BF     (ADD imm: Rn 28 -> 29)
"""
import struct, subprocess, sys

src = sys.argv[1]
dst = sys.argv[2]
data = bytearray(open(src, "rb").read())

e_shoff, = struct.unpack_from("<Q", data, 0x28)
e_shentsize, = struct.unpack_from("<H", data, 0x3A)
e_shnum, = struct.unpack_from("<H", data, 0x3C)

e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    p_type, = struct.unpack_from("<I", data, o)
    p_offset, p_vaddr, _, p_filesz = struct.unpack_from("<QQQQ", data, o + 8)
    if p_type == 1:
        loads.append((p_vaddr, p_offset, p_filesz))

def v2o(v):
    for vaddr, off, sz in loads:
        if vaddr <= v < vaddr + sz:
            return off + (v - vaddr)
    raise KeyError(hex(v))

sym = None
out = subprocess.run(["nm", "-D", "--defined-only", src], capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) >= 3 and p[2] == "art_quick_generic_jni_trampoline":
        sym = int(p[0], 16); break
if sym is None:
    # libart is stripped of this symbol; the tombstone gives it directly:
    # faulting pc 0x13f370 is trampoline+176, so the function starts 176 earlier.
    sym = 0x13f370 - 176
    print(f"symbol not exported; using tombstone-derived base {hex(sym)}")

target_v = sym + 172
off = v2o(target_v)
cur = struct.unpack_from("<I", data, off)[0]
print(f"art_quick_generic_jni_trampoline @ {hex(sym)}")
print(f"patch site  vaddr {hex(target_v)}  file offset {hex(off)}")
print(f"current word: {cur:#010x}")

MOV_SP_X28 = 0x9100039F
MOV_SP_X29 = 0x910003BF

if cur == MOV_SP_X29:
    print("already patched - nothing to do"); sys.exit(0)
if cur != MOV_SP_X28:
    print(f"FAIL: expected {MOV_SP_X28:#010x} (mov sp, x28), refusing to patch")
    sys.exit(1)

struct.pack_into("<I", data, off, MOV_SP_X29)
open(dst, "wb").write(data)
print(f"patched to  : {MOV_SP_X29:#010x}  (mov sp, x29)")
print(f"wrote {dst} ({len(data)} bytes)")

# verify by re-reading
chk = struct.unpack_from("<I", bytearray(open(dst,'rb').read()), off)[0]
print("verify:", "OK" if chk == MOV_SP_X29 else "FAILED")

#!/usr/bin/env python3
"""Apply the x28-recompute trampoline to any Pico SDK library.

usage: 242_patch_x28_generic.py <in.so> <out.so> <load_site_hex>

Every Pico SDK variant carries the same pvr_EnterVrMode shape:

    mov x28, x0          ; x28 = the hmdInfo struct from operator new
    add x27, x0, #328    ; x27 = struct + 0x148, from the SAME allocation
    ...
    bl  SetClientStatusCallback
    ldr x0, [x28, #8]    ; x28 comes back 0 here -> fault addr 0x8

x27 survives where x28 does not, so recompute the base from it. Verified working
in VRShell's bundled libPvr_UnitySDK.so; libPvr_UnitySDKExt11.so is byte-for-byte
the same pattern at the same +1336 offset.

Finds its own code cave (a run of zeros in an executable section that no symbol
covers) rather than hardcoding one, since each library differs.
"""
import struct, sys, shutil, subprocess

SRC, DST, LOAD_HEX = sys.argv[1], sys.argv[2], sys.argv[3]
LOAD_SITE = int(LOAD_HEX, 16)
RESUME = LOAD_SITE + 4
LDR_X0_X28_8 = 0xF9400780
SUB_X28_X27_148 = 0xD1000000 | (0x148 << 10) | (27 << 5) | 28

shutil.copyfile(SRC, DST)
data = bytearray(open(DST, "rb").read())

# section headers, to find executable ranges for the cave
e_shoff, = struct.unpack_from("<Q", data, 0x28)
e_shentsize, = struct.unpack_from("<H", data, 0x3A)
e_shnum, = struct.unpack_from("<H", data, 0x3C)
exec_ranges = []
for i in range(e_shnum):
    o = e_shoff + i * e_shentsize
    _, typ, flags, addr, off, size = struct.unpack_from("<IIQQQQ", data, o)
    if typ == 1 and (flags & 0x4):
        exec_ranges.append((addr, off, size))

cur = struct.unpack_from("<I", data, LOAD_SITE)[0]
print(f"load site {LOAD_SITE:#x}: {cur:#010x} (expect {LDR_X0_X28_8:#010x})")
if cur != LDR_X0_X28_8:
    print("FAIL: not the expected `ldr x0,[x28,#8]`, refusing to patch")
    sys.exit(1)

# find a zero run big enough for 3 instructions, 16-byte aligned, away from the site
cave = None
for addr, off, size in exec_ranges:
    run_start = None
    for i in range(off, off + size):
        if data[i] == 0:
            if run_start is None:
                run_start = i
        else:
            if run_start is not None and (i - run_start) >= 64:
                c = (run_start + 15) & ~15
                if abs(c - LOAD_SITE) > 64:
                    cave = c
                    break
            run_start = None
    if cave:
        break
if cave is None:
    print("FAIL: no code cave found")
    sys.exit(1)
print(f"cave at {cave:#x} ({sum(1 for b in data[cave:cave+64] if b == 0)}/64 bytes zero)")

def b_ins(frm, to):
    off = (to - frm) >> 2
    assert -(1 << 25) <= off < (1 << 25)
    return struct.pack("<I", 0x14000000 | (off & 0x03FFFFFF))

tramp = bytearray()
tramp += struct.pack("<I", SUB_X28_X27_148)
tramp += struct.pack("<I", LDR_X0_X28_8)
tramp += b_ins(cave + 8, RESUME)
data[cave:cave + len(tramp)] = tramp
data[LOAD_SITE:LOAD_SITE + 4] = b_ins(LOAD_SITE, cave)

open(DST, "wb").write(data)
print(f"patched: {LOAD_SITE:#x} -> {cave:#x} (sub x28,x27,#0x148 / ldr / b {RESUME:#x})")
print(f"wrote {DST}")

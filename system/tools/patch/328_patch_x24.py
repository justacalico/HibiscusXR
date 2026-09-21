#!/usr/bin/env python3
"""Recompute x28 from x24 instead of x27.

My original patch used `sub x28, x27, #0x148`, assuming x27 (set as `add x27, x0,
#0x148`) was intact. But at the crash x24 and x27 disagreed by 0x20, and I picked
x27 on the strength of the arithmetic. If x24 was the intact one instead, the
patch has been handing pvr_EnterVrMode a pointer 0x20 bytes off - enough to avoid
a crash, but every subsequent field write lands in the wrong place, which would
neatly explain why VR mode appears to succeed yet tracking never initialises.

x24 comes from `mov x24, x28`, a direct copy, so it needs no arithmetic:

    mov x28, x24        (ORR x28, xzr, x24 = 0xAA1803FC)
    ldr x0, [x28, #8]
    b   <resume>
"""
import struct, sys, shutil

SRC = sys.argv[1]
DST = sys.argv[2]
LOAD_SITE = 0x5ff58
RESUME = LOAD_SITE + 4
CAVE = 0xd00c0          # fresh cave, clear of the earlier trampolines at d0080/d00a0
LDR_X0_X28_8 = 0xF9400780
MOV_X28_X24 = 0xAA1803FC

shutil.copyfile(SRC, DST)
data = bytearray(open(DST, "rb").read())

cur = struct.unpack_from("<I", data, LOAD_SITE)[0]
print(f"load site {LOAD_SITE:#x}: {cur:#010x}")
# it currently holds our branch to the x27 trampoline; accept either that or the original
if cur == LDR_X0_X28_8:
    print("  (original ldr present)")
elif (cur & 0xFC000000) == 0x14000000:
    print("  (currently branches to the old x27 trampoline - repointing)")
else:
    print("  unexpected instruction, refusing"); sys.exit(1)

if any(data[CAVE:CAVE + 16]):
    print(f"FAIL: cave {CAVE:#x} not free"); sys.exit(1)

def b_ins(frm, to):
    off = (to - frm) >> 2
    return struct.pack("<I", 0x14000000 | (off & 0x03FFFFFF))

tramp = bytearray()
tramp += struct.pack("<I", MOV_X28_X24)
tramp += struct.pack("<I", LDR_X0_X28_8)
tramp += b_ins(CAVE + 8, RESUME)
data[CAVE:CAVE + len(tramp)] = tramp
data[LOAD_SITE:LOAD_SITE + 4] = b_ins(LOAD_SITE, CAVE)

open(DST, "wb").write(data)
print(f"cave {CAVE:#x}: mov x28,x24 / ldr x0,[x28,#8] / b {RESUME:#x}")
print(f"wrote {DST}")

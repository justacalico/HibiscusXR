#!/usr/bin/env python3
"""Recompute x28 from x27 at the faulting load in pvr_EnterVrMode.

Established so far:
  - single pvr_EnterVrMode entry (stock does three), hmdInfo all printed, so the
    allocation path ran and x28 held the struct
  - 0x5fce4 branches into the out-of-line block at 0x5ff24, which ends by
    branching back to 0x5fce8, so it is normal flow
  - protecting x28 across the LAST call (SetClientStatusCallback) changed nothing,
    so the clobber happens earlier in that block
  - x27 = A + 0x148 and x24 = A come from the same allocation, yet
    x27 - x24 = 0x128 in BOTH crash dumps - deterministic, so one of them is wrong

If x27 is the intact one, x28 can simply be recomputed as x27 - 0x148 right before
the load. That is one experiment which both tests the assumption and, if it holds,
gets VRShell past the crash:

    5ff58:  b   <cave2>
    cave2:  sub x28, x27, #0x148
            ldr x0, [x28, #8]
            b   0x5ff5c

If x27 is NOT intact this will fault on a wild pointer instead, which is itself a
clear answer.
"""
import struct, sys, shutil

SRC = sys.argv[1]
DST = sys.argv[2]

LOAD_SITE = 0x5ff58     # ldr x0, [x28, #8]
RESUME    = 0x5ff5c     # the instruction after it
CAVE2     = 0xd00a0     # further into the same dead gap, clear of the first trampoline

shutil.copyfile(SRC, DST)
data = bytearray(open(DST, "rb").read())

def b_ins(frm, to):
    off = (to - frm) >> 2
    assert -(1 << 25) <= off < (1 << 25)
    return struct.pack("<I", 0x14000000 | (off & 0x03FFFFFF))

orig = struct.unpack_from("<I", data, LOAD_SITE)[0]
LDR_X0_X28_8 = 0xF9400780
print(f"load site {LOAD_SITE:#x}: {orig:#010x} (expect {LDR_X0_X28_8:#010x})")
if orig != LDR_X0_X28_8:
    print("FAIL: not the expected ldr, refusing to patch")
    sys.exit(1)

if any(data[CAVE2:CAVE2 + 16]):
    print(f"FAIL: cave2 at {CAVE2:#x} not zero-filled")
    sys.exit(1)

# sub x28, x27, #0x148  ->  SUB (immediate), sf=1: 0xD1000000 | imm12<<10 | Rn<<5 | Rd
SUB = 0xD1000000 | (0x148 << 10) | (27 << 5) | 28
tramp = bytearray()
tramp += struct.pack("<I", SUB)
tramp += struct.pack("<I", LDR_X0_X28_8)
tramp += b_ins(CAVE2 + 8, RESUME)
data[CAVE2:CAVE2 + len(tramp)] = tramp

data[LOAD_SITE:LOAD_SITE + 4] = b_ins(LOAD_SITE, CAVE2)

open(DST, "wb").write(data)
print(f"cave2 at {CAVE2:#x}: sub x28,x27,#0x148 / ldr x0,[x28,#8] / b {RESUME:#x}")
print(f"load site now branches to {CAVE2:#x}")
print(f"wrote {DST}")

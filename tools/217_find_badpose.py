#!/usr/bin/env python3
"""Locate the "Bad Pose.Orientation" check in the Pico SDK and show what it tests.

Every warp frame is rejected with:
    DIATW SelectRT Bad Pose.Orientation in bufferNum N!
    DIATW SelectRT Nothing to draw, draw black!

Head tracking itself is fine (pvrservice emits valid unit quaternions), so the
pose is being lost between the sensor and the compositor. Find the string, find
the adrp/add pair that references it, and print the surrounding code so the
validation and its operand are visible.
"""
import struct, sys

path = sys.argv[1] if len(sys.argv) > 1 else r"F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.so"
NEEDLE = b"Bad Pose.Orientation"

data = open(path, "rb").read()

# where does the string live
pos = data.find(NEEDLE)
if pos < 0:
    print("string not found"); sys.exit(1)
# walk back to the start of the C string
start = data.rfind(b"\0", 0, pos) + 1
print(f'string "{data[start:data.find(b chr(0).encode(), start)].decode(errors="replace") if False else data[start:data.index(b"\\x00", start)].decode(errors="replace")}"')
print(f"  file offset {start:#x}")

# vaddr == file offset for the PT_LOAD segments in this object; confirm
e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
vaddr = None
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    t, = struct.unpack_from("<I", data, o)
    po, pv, _, pf = struct.unpack_from("<QQQQ", data, o + 8)
    if t == 1 and po <= start < po + pf:
        vaddr = pv + (start - po)
        break
print(f"  vaddr {vaddr:#x}")

# find adrp/add pairs that compute it, scanning the executable range
page = vaddr & ~0xFFF
lo = vaddr & 0xFFF
hits = []
for off in range(0, len(data) - 8, 4):
    w1 = struct.unpack_from("<I", data, off)[0]
    if (w1 & 0x9F000000) != 0x90000000:      # ADRP
        continue
    rd = w1 & 0x1F
    immlo = (w1 >> 29) & 0x3
    immhi = (w1 >> 5) & 0x7FFFF
    imm = ((immhi << 2) | immlo)
    if imm & (1 << 20):
        imm -= (1 << 21)
    target_page = ((off & ~0xFFF) + (imm << 12)) & 0xFFFFFFFFFFFF
    if target_page != page:
        continue
    # look ahead a few instructions for an ADD of the low bits into the same reg
    for k in range(1, 8):
        w2 = struct.unpack_from("<I", data, off + 4 * k)[0]
        if (w2 & 0xFF800000) == 0x91000000:   # ADD immediate, 64-bit
            rn = (w2 >> 5) & 0x1F
            imm12 = (w2 >> 10) & 0xFFF
            if rn == rd and imm12 == lo:
                hits.append(off)
                break

print(f"\nreferenced from {len(hits)} site(s):")
for h in hits:
    print(f"  {h:#x}")

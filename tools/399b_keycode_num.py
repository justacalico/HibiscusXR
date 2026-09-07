#!/usr/bin/env python3
"""Same goal, but the table stores its pointers in place: scan the file for an
8-byte little-endian word equal to each label's string vaddr, then read the
int32 that follows it."""
import struct, re

LIB = r"F:\PN2Lineage\notes\stock_system_lib64_libinput.so"
WANTED = ["HOME", "BACK", "ALL_APPS", "DC_IN", "HALL_OPEN", "HALL_CLOSE",
          "DEFINE_CONFIRM", "DEFINE_CONTROLLER_CONFIRM", "CAMERA", "FOCUS"]

data = open(LIB, "rb").read()

# find each label string, preceded and followed by NUL
vaddrs = {}
for w in WANTED:
    pat = b"\0" + w.encode() + b"\0"
    i = data.find(pat)
    if i >= 0:
        vaddrs[w] = i + 1          # file offset; for this lib vaddr == offset in .rodata

print(f"{'label':<28} {'str off':>10} {'slot':>10}  value")
for w in WANTED:
    v = vaddrs.get(w)
    if v is None:
        print(f"{w:<28} {'not found':>10}")
        continue
    # .rodata here sits 0x8000 higher in vaddr than in the file
    hits = []
    for cand in (v, v + 0x8000):
        needle = struct.pack("<Q", cand)
        hits += [m.start() for m in re.finditer(re.escape(needle), data)]
    if not hits:
        print(f"{w:<28} {v:>#10x} {'no slot':>10}")
        continue
    for h in hits[:3]:
        val = struct.unpack_from("<i", data, h + 8)[0]
        print(f"{w:<28} {v:>#10x} {h:>#10x}  {val}")

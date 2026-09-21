#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Find the real vptr offset inside _ZTVN3PVR12BpPvrServiceE.

BpPvrService has a VTT, so it has virtual bases and the vtable begins with
vbase offsets before offset-to-top/RTTI. That means vptr is NOT vtable+8, and
my slot arithmetic was wrong. Locate the BitTube method (0xf058) and the int32
method (0xefc8) in the table and derive the offset that makes the call sites
consistent.
"""
import subprocess, re

LIB = PN2_ROOT + "/notes/pvrsc32.so"
OBJDUMP = os.environ.get("OBJDUMP", "llvm-objdump")
VT, VT_SIZE = 0x194D0, 0x16C

mem = {}
out = subprocess.run([OBJDUMP, "-s", "-j", ".data.rel.ro", LIB],
                     capture_output=True, text=True).stdout
for line in out.splitlines():
    m = re.match(r"^\s*([0-9a-f]+)\s+((?:[0-9a-f]{8}\s+){1,4})", line)
    if not m:
        continue
    base = int(m.group(1), 16)
    for i, w in enumerate(m.group(2).split()):
        mem[base + i * 4] = int.from_bytes(bytes.fromhex(w), "little")

print("full vtable:")
entries = []
for i in range(VT_SIZE // 4):
    a = VT + i * 4
    v = mem.get(a, 0)
    entries.append((a, v))
    print(f"  [{i:>3}] {a:#07x} = {v:#010x}")

# the two methods we identified by disassembly
for target, what in ((0xF059, "BitTube method (code 0x3b)"),
                     (0xEFC9, "int32 method (code 0x3f)")):
    for a, v in entries:
        if v == target:
            print(f"\n{what} at vtable {a:#x}")
            for hdr in (8, 12, 16, 20, 24):
                print(f"   if vptr=vtable+{hdr:<3} -> slot offset {a - (VT + hdr):#x}")

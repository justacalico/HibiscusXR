#!/usr/bin/env python3
"""Resolve a PLT stub address to the symbol it imports.

pvr_EnterVrMode crashes at `bl #0xf6ec0`. If that PLT entry belongs to a library
we are still missing (or a symbol that resolved to null), that is the next gap.
"""
import struct, subprocess, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = "/mnt/f/PN2Lineage/notes/vrshell_lib/libPvr_UnitySDK.so"
targets = [int(x, 16) for x in (sys.argv[1:] or ["0xf6ec0", "0xf7248", "0x6c818"])]
data = open(path, "rb").read()

e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    t, = struct.unpack_from("<I", data, o)
    po, pv, _, pf = struct.unpack_from("<QQQQ", data, o + 8)
    if t == 1: loads.append((pv, po, pf))
def v2o(v):
    for pv, po, pf in loads:
        if pv <= v < pv + pf: return po + (v - pv)
    return None

# map GOT slot -> symbol name from the relocations
rel = {}
out = subprocess.run(["readelf", "-rW", path], capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) >= 5 and p[0].startswith("0000"):
        try: off = int(p[0], 16)
        except ValueError: continue
        name = p[4] if len(p) > 4 else ""
        if len(p) >= 5 and "@" not in name and not name.startswith("0x"):
            rel[off] = p[-1]

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
for t in targets:
    off = v2o(t)
    if off is None:
        print(f"{hex(t)}: not mapped"); continue
    print(f"\n=== stub at {hex(t)} ===")
    page = None
    for ins in md.disasm(data[off:off + 24], t):
        print(f"  {ins.address:08x}  {ins.mnemonic:<8} {ins.op_str}")
        if ins.mnemonic == "adrp":
            page = int(ins.op_str.split(",")[1].strip().lstrip("#"), 16)
        elif ins.mnemonic == "ldr" and page is not None and "[" in ins.op_str:
            inside = ins.op_str.split("[")[1].rstrip("]")
            parts = [x.strip() for x in inside.split(",")]
            disp = int(parts[1].lstrip("#"), 16) if len(parts) > 1 else 0
            got = page + disp
            nm = rel.get(got)
            print(f"     -> GOT {hex(got)} = {nm if nm else '(no relocation found)'}")

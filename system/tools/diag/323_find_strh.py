#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Find every 16-bit store at offset 0x570/0x572, whatever base register is used.

getTrackingState reads:
    ldrh w3, [x19, #0x570]
    ldrh w4, [x19, #0x572]
with x19 = 0x2049b0. My earlier search only looked at code that materialised that
base with adrp+add, so a write through a register held across calls would have been
missed. Scan for the STRH encoding directly.

STRH (immediate, unsigned offset): 0x79000000 | (imm12 << 10) | (Rn << 5) | Rt
where imm12 = byteoffset / 2. 0x570/2 = 0x2b8, 0x572/2 = 0x2b9.
"""
import struct
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

L = PN2_ROOT + "/notes/vrshell_lib/libPvr_UnitySDK.so"
data = open(L, "rb").read()

e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    t, = struct.unpack_from("<I", data, o)
    po, pv, _, pf = struct.unpack_from("<QQQQ", data, o + 8)
    if t == 1: loads.append((pv, po, pf))
def o2v(off):
    for pv, po, pf in loads:
        if po <= off < po + pf: return pv + (off - po)
    return None
def v2o(v):
    for pv, po, pf in loads:
        if pv <= v < pv + pf: return po + (v - pv)
    return None

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
hits = []
for off in range(0, len(data) - 4, 4):
    w = struct.unpack_from("<I", data, off)[0]
    # STRH imm, and also STR (32-bit) at 0x570 in case it writes both at once
    is_strh = (w & 0xFFC00000) == 0x79000000 and ((w >> 10) & 0xFFF) in (0x2b8, 0x2b9)
    is_str  = (w & 0xFFC00000) == 0xB9000000 and ((w >> 10) & 0xFFF) == (0x570 // 4)
    if not (is_strh or is_str): continue
    va = o2v(off)
    if va is None: continue
    hits.append((va, "strh" if is_strh else "str"))

print(f"{len(hits)} candidate stores at +0x570/+0x572\n")
for va, kind in hits[:12]:
    print(f"===== {va:#x} ({kind}) =====")
    lo = va - 0x40
    code = data[v2o(lo): v2o(va) + 0x18]
    for ins in md.disasm(code, lo):
        mark = "   <== the store" if ins.address == va else ""
        print(f"  {ins.address:08x}  {ins.mnemonic:<9} {ins.op_str}{mark}")
    print()

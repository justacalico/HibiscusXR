#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Who writes the tracking-state fields the SDK reports?

The logger just reads two u16s from a global (base 0x2049b0, +0x570/+0x572) - it
is a cached value, not a live query. Find every store to that base+offset so we
can see what is meant to populate it.
"""
import struct
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

L = PN2_ROOT + "/notes/vrshell_lib/libPvr_UnitySDK.so"
BASE = 0x2049b0
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

# find code that materialises the BASE pointer (adrp+add), then stores at +0x570
page = BASE & ~0xFFF
lo = BASE & 0xFFF
md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)

sites = []
for off in range(0, len(data) - 8, 4):
    w1 = struct.unpack_from("<I", data, off)[0]
    if (w1 & 0x9F000000) != 0x90000000: continue
    rd = w1 & 0x1F
    imm = (((w1 >> 5) & 0x7FFFF) << 2) | ((w1 >> 29) & 3)
    if imm & (1 << 20): imm -= (1 << 21)
    va = o2v(off)
    if va is None: continue
    if ((va & ~0xFFF) + (imm << 12)) != page: continue
    for k in range(1, 8):
        w2 = struct.unpack_from("<I", data, off + 4*k)[0]
        if (w2 & 0xFF800000) == 0x91000000 and ((w2 >> 5) & 0x1F) == rd and ((w2 >> 10) & 0xFFF) == lo:
            sites.append((va, rd)); break

print(f"{len(sites)} sites materialise the global\n")
found = 0
for va, reg in sites:
    code = data[v2o(va): v2o(va) + 0x140]
    for ins in md.disasm(code, va):
        # a store into that base at +0x570/+0x572
        if ins.mnemonic.startswith("str") and "#0x57" in ins.op_str and f"x{reg}" in ins.op_str:
            print(f"  WRITE at {ins.address:#x}: {ins.mnemonic} {ins.op_str}")
            found += 1
            # show a little context
            ctx = data[v2o(ins.address - 0x30): v2o(ins.address) + 0x20]
            for c in md.disasm(ctx, ins.address - 0x30):
                mk = "  <==" if c.address == ins.address else ""
                print(f"      {c.address:08x}  {c.mnemonic:<9} {c.op_str}{mk}")
            print()
if not found:
    print("  no direct store found near those sites - the field is probably written")
    print("  through a pointer held in a register (e.g. memcpy of a struct)")

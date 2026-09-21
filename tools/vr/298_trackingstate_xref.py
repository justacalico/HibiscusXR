#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Find the code that logs "trackingstate = 0x%x,0x%x" and what it reads.

I assumed this came from QVR because the SDK contains QVRServiceClient strings.
It does not: VRShell loads no libqvr* library at all. So the two values come from
somewhere else - find the adrp/add pair that references the format string and
disassemble around it.
"""
import struct, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

L = PN2_ROOT + "/notes/vrshell_lib/libPvr_UnitySDK.so"
data = open(L, "rb").read()

NEEDLE = b"trackingstate = 0x%x,0x%x"
pos = data.find(NEEDLE)
if pos < 0:
    print("string not found"); sys.exit(1)
start = data.rfind(b"\x00", 0, pos) + 1

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

vaddr = o2v(start)
print(f'string at vaddr {vaddr:#x}')

page = vaddr & ~0xFFF
lo = vaddr & 0xFFF
hits = []
for off in range(0, len(data) - 8, 4):
    w1 = struct.unpack_from("<I", data, off)[0]
    if (w1 & 0x9F000000) != 0x90000000: continue
    rd = w1 & 0x1F
    imm = (((w1 >> 5) & 0x7FFFF) << 2) | ((w1 >> 29) & 3)
    if imm & (1 << 20): imm -= (1 << 21)
    va = o2v(off)
    if va is None: continue
    if ((va & ~0xFFF) + (imm << 12)) != page: continue
    for k in range(1, 10):
        w2 = struct.unpack_from("<I", data, off + 4*k)[0]
        if (w2 & 0xFF800000) == 0x91000000 and ((w2 >> 5) & 0x1F) == rd and ((w2 >> 10) & 0xFFF) == lo:
            hits.append(va); break

print(f"referenced from {len(hits)} site(s): {[hex(h) for h in hits]}")

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
for h in hits[:2]:
    lo_a = h - 0x90
    print(f"\n===== around {h:#x} =====")
    code = data[v2o(lo_a):v2o(h) + 0x40]
    for ins in md.disasm(code, lo_a):
        mark = "   <== the log call site" if ins.address == h else ""
        print(f"  {ins.address:08x}  {ins.mnemonic:<9} {ins.op_str}{mark}")

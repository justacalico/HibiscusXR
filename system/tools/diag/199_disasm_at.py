#!/usr/bin/env python3
"""Disassemble a window around <lib> <offset>, resolving string literals.

usage: 199_disasm_at.py <lib.so> <hex-offset> [bytes-before] [bytes-after]

Used for pvr_EnterVrMode+1336, where VRShell now dies with fault addr 0x8 and
x0 = 0 - a load at +8 off a null base, immediately before the point where stock
prints hmdInfo.lensSeparation.
"""
import struct, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = sys.argv[1]
target = int(sys.argv[2], 16)
before = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x90
after = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0x30
data = open(path, "rb").read()

e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    t, = struct.unpack_from("<I", data, o)
    po, pv, _, pf = struct.unpack_from("<QQQQ", data, o + 8)
    if t == 1:
        loads.append((pv, po, pf))
def v2o(v):
    for pv, po, pf in loads:
        if pv <= v < pv + pf:
            return po + (v - pv)
    return None
def cstr(a, m=200):
    o = v2o(a)
    if o is None or o >= len(data): return None
    e = data.find(b"\0", o, o + m)
    if e <= o: return None
    try: s = data[o:e].decode("utf-8")
    except UnicodeDecodeError: return None
    return s if all(32 <= ord(c) < 127 for c in s) else None

start = target - before
md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
off = v2o(start)
code = data[off:off + before + after]
print(f"{path}\nwindow {hex(start)} .. {hex(target + after)}   target {hex(target)}\n")
pend = {}
for ins in md.disasm(code, start):
    line = f"  {ins.address:08x}  {ins.mnemonic:<9} {ins.op_str}"
    if ins.mnemonic == "adrp":
        r, imm = [x.strip() for x in ins.op_str.split(",")]
        pend[r] = int(imm.lstrip("#"), 16)
    elif ins.mnemonic == "add":
        p = [x.strip() for x in ins.op_str.split(",")]
        if len(p) == 3 and p[1] in pend and p[2].startswith("#"):
            s = cstr(pend[p[1]] + int(p[2][1:], 16))
            if s: line += f'        ; "{s}"'
    if ins.address == target:
        line += "   <=========== FAULTING INSTRUCTION"
    print(line)

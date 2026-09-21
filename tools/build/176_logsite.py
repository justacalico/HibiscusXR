#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Show the __android_log_print call site in libpvrservice.so that crashes.

Tombstone frame #04 is  libpvrservice.so + 0xd0d4  -- the return address, so the
argument setup for the log call sits just before it. Resolve adrp/add pairs into
string literals so the format string and any %s operands are visible, and mark
which registers are still live from a null source.
"""
import struct, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = PN2_ROOT + "/notes/lib64/libpvrservice.so"
RET  = 0xd0d4
START = RET - 0x150
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

def cstr(addr, maxlen=160):
    o = v2o(addr)
    if o is None or o >= len(data):
        return None
    end = data.find(b"\0", o, o + maxlen)
    if end <= o:
        return None
    try:
        s = data[o:end].decode("utf-8")
    except UnicodeDecodeError:
        return None
    return s if all(32 <= ord(c) < 127 or c in "\t\n" for c in s) else None

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
off = v2o(START)
code = data[off:v2o(RET) + 16]

print(f"libpvrservice.so  window {hex(START)} .. {hex(RET)}")
print("(frame #04 returns to +0xd0d4, so the log-call setup is just above it)\n")

pend = {}
for ins in md.disasm(code, START):
    line = f"  {ins.address:06x}  {ins.mnemonic:<8} {ins.op_str}"
    if ins.mnemonic == "adrp":
        r, imm = [x.strip() for x in ins.op_str.split(",")]
        pend[r] = int(imm.lstrip("#"), 16)
    elif ins.mnemonic == "add":
        parts = [x.strip() for x in ins.op_str.split(",")]
        if len(parts) == 3 and parts[1] in pend and parts[2].startswith("#"):
            a = pend[parts[1]] + int(parts[2][1:], 16)
            s = cstr(a)
            if s:
                line += f'        ; {parts[0]} = "{s}"'
    if ins.address == RET - 4:
        line += "   <=== the crashing call"
    if ins.address == RET:
        line += "   <=== returns here (+0xd0d4)"
    print(line)

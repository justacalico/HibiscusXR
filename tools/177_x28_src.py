#!/usr/bin/env python3
"""Find where x28 (the format string for the crashing log call) is assigned.

At +0xd0cc the code does `mov x2, x28` and calls __android_log_print, so x28 IS
the format string. pvrservice's tombstone has x28 = 0, hence a null format and the
fault at 0x0.

In a plain native process there is no JNI trampoline in play, so the likely story
is simply that x28 is set on some paths and not others -- i.e. a branch that stock
takes and we do not. Sweep backwards from the call for every write to x28 and
resolve any string literal it is loaded with.
"""
import struct
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = "/mnt/f/PN2Lineage/notes/lib64/libpvrservice.so"
CALL  = 0xd0d4
START = 0xcae0          # well before the call; covers the whole function
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
def cstr(addr, maxlen=200):
    o = v2o(addr)
    if o is None or o >= len(data): return None
    end = data.find(b"\0", o, o + maxlen)
    if end <= o: return None
    try: s = data[o:end].decode("utf-8")
    except UnicodeDecodeError: return None
    return s if all(32 <= ord(c) < 127 for c in s) else None

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
off = v2o(START)
code = data[off:v2o(CALL) + 8]

pend = {}
print(f"writes to x28 between {hex(START)} and {hex(CALL)}:\n")
found = 0
for ins in md.disasm(code, START):
    ops = ins.op_str
    if ins.mnemonic == "adrp":
        r, imm = [x.strip() for x in ops.split(",")]
        pend[r] = int(imm.lstrip("#"), 16)
        continue
    parts = [x.strip() for x in ops.split("[")[0].split(",")]
    if not parts or parts[0] != "x28":
        continue
    found += 1
    line = f"  {ins.address:06x}  {ins.mnemonic:<8} {ops}"
    if ins.mnemonic == "add" and len(parts) == 3 and parts[1] in pend and parts[2].startswith("#"):
        a = pend[parts[1]] + int(parts[2][1:], 16)
        s = cstr(a)
        if s: line += f'        ; "{s}"'
    if ins.mnemonic in ("mov",) and parts[1] == "xzr":
        line += "        ; <-- explicitly set to NULL"
    print(line)
if not found:
    print("  (none - x28 is never written in this range, so it arrives already zero)")

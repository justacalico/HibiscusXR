#!/usr/bin/env python3
"""Linear sweep of .text for writes to x28, ignoring symbols.

The libraries are stripped, so a per-symbol scan covers almost nothing. Sweep the
whole executable range instead and report every instruction that WRITES x28,
along with whether the surrounding region ever saves it.

Under AAPCS64 x28 is callee-saved, so well-behaved compiler output only touches it
after an stp/str that spills it. Writes with no nearby save are the ABI violations
that make Android 10's generic JNI trampoline restore sp = garbage.
"""
import struct, sys, re
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = sys.argv[1]
data = open(path, "rb").read()
if data[:4] != b"\x7fELF" or data[4] != 2:
    sys.exit(0)

# section headers -> find executable sections
e_shoff, = struct.unpack_from("<Q", data, 0x28)
e_shentsize, = struct.unpack_from("<H", data, 0x3A)
e_shnum, = struct.unpack_from("<H", data, 0x3C)
e_shstrndx, = struct.unpack_from("<H", data, 0x3E)
def sh(i):
    o = e_shoff + i * e_shentsize
    name, typ, flags, addr, off, size = struct.unpack_from("<IIQQQQ", data, o)
    return name, typ, flags, addr, off, size
_, _, _, _, stroff, _ = sh(e_shstrndx)
def sname(n):
    end = data.index(b"\0", stroff + n)
    return data[stroff + n:end].decode()

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
total_writes = 0
saves = 0
hits = []

for i in range(e_shnum):
    name, typ, flags, addr, off, size = sh(i)
    if typ != 1 or not (flags & 0x4):   # PROGBITS + EXECINSTR
        continue
    nm = sname(name)
    code = data[off:off + size]
    # decode in one pass
    for ins in md.disasm(code, addr):
        ops = ins.op_str
        if "x28" not in ops:
            continue
        m = ins.mnemonic
        if m.startswith(("stp", "str")):
            saves += 1
            continue
        # destination position: first operand, or second for stp-style pairs
        parts = [p.strip() for p in ops.split("[")[0].split(",")]
        writes = False
        if m.startswith(("ldp",)):
            writes = "x28" in parts[:2]
        elif m.startswith(("ld",)):
            writes = parts and parts[0] == "x28"
        else:
            writes = parts and parts[0] == "x28"
        if writes:
            total_writes += 1
            # ldp restoring x28 is the normal epilogue; flag only non-load writes
            if not m.startswith("ld"):
                hits.append((nm, ins.address, m, ops))

print(f"\n### {path}")
print(f"  x28 saves (stp/str): {saves}")
print(f"  x28 writes total   : {total_writes}")
print(f"  non-load writes    : {len(hits)}")
for nm, a, m, ops in hits[:60]:
    print(f"    {nm} {a:#x}  {m} {ops}")

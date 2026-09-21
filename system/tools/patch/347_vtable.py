#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Map BpPvrService vtable slots to method names.

getPvrService calls vtable[+0x104] (registerClient, which we see succeed) and
then vtable[+0xf4], which is the one that returns sp<BitTube> and after which
the caller's r4 comes back as 0. Naming that slot tells us what actually runs.
"""
import subprocess, struct, sys, re

LIB = PN2_ROOT + "/notes/pvrsc32.so"
OBJDUMP = os.environ.get("OBJDUMP", "llvm-objdump")
VT = 0x194D0
VT_SIZE = 0x16C

data = open(LIB, "rb").read()

# symbol table: addr -> name (thumb bit cleared)
syms = {}
out = subprocess.run([OBJDUMP, "-T", LIB], capture_output=True, text=True).stdout
for line in out.splitlines():
    m = re.match(r"^([0-9a-f]{8})\s+\S.*?\s(\S+)$", line.strip())
    if m:
        syms.setdefault(int(m.group(1), 16) & ~1, m.group(2))

print(f"vtable at {VT:#x}, {VT_SIZE} bytes = {VT_SIZE//4} words\n")
print(f"{'slot':>5} {'vptr_off':>9} {'value':>10}  symbol")
for i in range(VT_SIZE // 4):
    off = VT + i * 4
    val = struct.unpack_from("<I", data, off)[0]
    vptr_off = off - (VT + 8)          # offset as used from the vptr
    name = syms.get(val & ~1, "")
    mark = ""
    if vptr_off == 0xF4:
        mark = "   <<< CRASHES HERE"
    elif vptr_off == 0x104:
        mark = "   <<< registerClient (works)"
    if name or mark:
        print(f"{i:>5} {vptr_off:>+#9x} {val:>#10x}  {name}{mark}")

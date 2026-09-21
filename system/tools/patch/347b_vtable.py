#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Name the BpPvrService vtable slots, reading .data.rel.ro by VMA."""
import subprocess, re

LIB = PN2_ROOT + "/notes/pvrsc32.so"
OBJDUMP = os.environ.get("OBJDUMP", "llvm-objdump")
VPTR = 0x194D0 + 8

mem = {}
out = subprocess.run([OBJDUMP, "-s", "-j", ".data.rel.ro", LIB],
                     capture_output=True, text=True).stdout
for line in out.splitlines():
    m = re.match(r"^\s*([0-9a-f]+)\s+((?:[0-9a-f]{8}\s+){1,4})", line)
    if not m:
        continue
    base = int(m.group(1), 16)
    for i, w in enumerate(m.group(2).split()):
        # objdump prints raw bytes; swap to little-endian value
        b = bytes.fromhex(w)
        mem[base + i * 4] = int.from_bytes(b, "little")

syms = {}
out = subprocess.run([OBJDUMP, "-T", LIB], capture_output=True, text=True).stdout
for line in out.splitlines():
    m = re.match(r"^([0-9a-f]{8})\s+\S.*?\s(\S+)$", line.strip())
    if m:
        syms.setdefault(int(m.group(1), 16) & ~1, m.group(2))

for off in (0xF4, 0x104, 0xFC, 0x10C, 0xEC):
    v = mem.get(VPTR + off)
    name = syms.get(v & ~1, "<no symbol>") if v else "<not mapped>"
    tag = {0xF4: "  <<< CRASHES HERE", 0x104: "  <<< registerClient (works)"}.get(off, "")
    print(f"vptr+{off:#05x} -> {v:#010x}  {name}{tag}" if v else f"vptr+{off:#05x} -> ?")

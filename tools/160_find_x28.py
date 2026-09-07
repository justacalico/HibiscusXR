#!/usr/bin/env python3
"""Find Pico native functions that clobber x28 (callee-saved under AAPCS64).

Android 10's art_quick_generic_jni_trampoline keeps the caller's stack pointer in
x28 across a JNI native call:

    +68   mov x28, sp
    +144  blr x16              <-- the native method
    +172  mov sp, x28          <-- restores sp FROM x28
    +176  ldp d0, d1, [sp,#0x10]   <-- faults if x28 came back wrong

The tombstone has x28 = 0 and fault addr 0x10, so some native function returns
without preserving x28. On 8.1 the trampoline stashed sp elsewhere, which is why
the same binary was fine there.

A function is flagged when it WRITES x28 but never saves it (no stp/str of x28
into its frame). Those are the ABI violators.
"""
import struct, subprocess, sys, re
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

path = sys.argv[1]
only_java = "--all" not in sys.argv
data = open(path, "rb").read()
if data[:4] != b"\x7fELF" or data[4] != 2:
    print(f"{path}: not ELF64, skipping"); sys.exit(0)

e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    p_type, = struct.unpack_from("<I", data, o)
    p_offset, p_vaddr, _, p_filesz = struct.unpack_from("<QQQQ", data, o + 8)
    if p_type == 1:
        loads.append((p_vaddr, p_offset, p_filesz))

def v2o(v):
    for vaddr, off, sz in loads:
        if vaddr <= v < vaddr + sz:
            return off + (v - vaddr)
    return None

# symbol table with sizes
syms = []
out = subprocess.run(["readelf", "-sW", path], capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) >= 8 and p[3] == "FUNC":
        try:
            addr = int(p[1], 16); size = int(p[2]); name = p[7]
        except ValueError:
            continue
        if size and addr:
            syms.append((name, addr, size))

seen = set()
md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
flagged = []
for name, addr, size in syms:
    if (name, addr) in seen: continue
    seen.add((name, addr))
    if only_java and not name.startswith("Java_"): continue
    off = v2o(addr)
    if off is None: continue
    code = data[off:off + min(size, 200000)]
    writes_x28 = False
    saves_x28 = False
    first_write = None
    for ins in md.disasm(code, addr):
        m, ops = ins.mnemonic, ins.op_str
        # a save looks like  stp x27, x28, [sp,...]  /  str x28, [sp,...]
        if m.startswith("st") and "x28" in ops:
            saves_x28 = True
        # a write is x28 appearing as the destination operand
        elif re.match(r"^\s*x28\s*,", ops) or ops.startswith("x28,") or \
             (m.startswith("ld") and re.search(r"\bx28\b", ops.split("[")[0])):
            writes_x28 = True
            if first_write is None: first_write = ins
    if writes_x28 and not saves_x28:
        flagged.append((name, addr, size, first_write))

if flagged:
    print(f"\n### {path}")
    for name, addr, size, ins in flagged:
        print(f"  CLOBBERS x28: {name}  @ {hex(addr)} (size {size})")
        if ins:
            print(f"     first write: {hex(ins.address)}  {ins.mnemonic} {ins.op_str}")
else:
    if "--quiet" not in sys.argv:
        print(f"  {path}: no x28-clobbering {'Java_*' if only_java else ''} functions")

#!/usr/bin/env python3
"""Disassemble art_quick_generic_jni_trampoline around the faulting offset.

Tombstone:
    pc  = art_quick_generic_jni_trampoline + 176
    sp  = 0            <-- stack pointer is ZERO
    x20 = 0
    fault addr 0x10    <-- consistent with a load at [x20, #0x10]

On aarch64 this trampoline calls artQuickGenericJniTrampoline(Thread*, SP), which
returns the new stack pointer, and the assembly then installs it. If that helper
returns null the stack pointer becomes null. So the question is what the code does
at +176 and which register it trusts.
"""
import struct, subprocess, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN

SO = sys.argv[1] if len(sys.argv) > 1 else "/mnt/f/PN2Lineage/notes/libart.so"
data = open(SO, "rb").read()

assert data[:4] == b"\x7fELF" and data[4] == 2, "expected ELF64"
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
    raise KeyError(hex(v))

sym = None
for tool in ("nm -D --defined-only", "nm --defined-only"):
    out = subprocess.run(tool.split() + [SO], capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) >= 3 and p[2] == "art_quick_generic_jni_trampoline":
            sym = int(p[0], 16); break
    if sym: break

if sym is None:
    print("symbol not found; falling back to the tombstone offset 0x13f370-176")
    sym = 0x13f370 - 176

print(f"art_quick_generic_jni_trampoline @ {hex(sym)}")
print(f"faulting pc = +176 = {hex(sym + 176)}\n")

md = Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN)
off = v2o(sym)
code = data[off:off + 320]
for ins in md.disasm(code, sym):
    delta = ins.address - sym
    mark = ""
    if delta == 176:
        mark = "   <=========== FAULTING INSTRUCTION (pc)"
    elif delta == 172:
        mark = "   <-- lr points here (the call before)"
    elif "x20" in ins.op_str and ("ldr" in ins.mnemonic or "str" in ins.mnemonic):
        mark = "   (touches x20, which is 0)"
    elif ins.mnemonic == "mov" and ins.op_str.startswith("sp,"):
        mark = "   *** installs a new stack pointer ***"
    print(f"  +{delta:<4} {ins.address:08x}  {ins.mnemonic:<10} {ins.op_str}{mark}")
print("\nDONE")

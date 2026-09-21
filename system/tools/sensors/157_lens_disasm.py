#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""Disassemble the Pico SDK functions that every VR app dies inside.

The crash is SIGSEGV with fault addr 0x10. That is a load at offset 0x10 from a
null base register, i.e. something returned null and the SDK read a field out of
it without checking. The candidates are the two Java upcalls:

    PvrClientJava::getLensInfo(PVR::_lensParameters*)
    PvrClientJava::getDisplayInfo(PVR::_displayInfo*)

both of which call a Java method returning a com.pvr.pvrservice.* object and then
pull fields off the result. Print their disassembly and flag every load at #0x10.
"""
import struct, subprocess, sys, re
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_ARM, CS_MODE_LITTLE_ENDIAN

SO = PN2_ROOT + "/ref/alvr-pico-legacy/app/src/main/jniLibs/armeabi-v7a/libPvr_UnitySDK.so"

data = open(SO, "rb").read()

# --- minimal ELF32 program-header walk: vaddr -> file offset -----------------
assert data[:4] == b"\x7fELF" and data[4] == 1, "not ELF32"
e_phoff, = struct.unpack_from("<I", data, 0x1C)
e_phentsize, = struct.unpack_from("<H", data, 0x2A)
e_phnum, = struct.unpack_from("<H", data, 0x2C)
loads = []
for i in range(e_phnum):
    o = e_phoff + i * e_phentsize
    p_type, p_offset, p_vaddr, _, p_filesz = struct.unpack_from("<IIIII", data, o)
    if p_type == 1:
        loads.append((p_vaddr, p_offset, p_filesz))

def v2o(v):
    for vaddr, off, sz in loads:
        if vaddr <= v < vaddr + sz:
            return off + (v - vaddr)
    raise KeyError(hex(v))

# --- symbols ----------------------------------------------------------------
syms = {}
out = subprocess.run(["nm", "-D", "--defined-only", SO],
                     capture_output=True, text=True).stdout
for line in out.splitlines():
    p = line.split()
    if len(p) >= 3:
        try: syms[p[2]] = int(p[0], 16)
        except ValueError: pass

def find(sub):
    for n, a in syms.items():
        if sub in n:
            return n, a
    return None, None

md_t = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_LITTLE_ENDIAN)
md_a = Cs(CS_ARCH_ARM, CS_MODE_ARM | CS_MODE_LITTLE_ENDIAN)

def dis(sub, nbytes=560):
    name, addr = find(sub)
    if not name:
        print(f"\n### {sub}: NOT FOUND"); return
    thumb = addr & 1
    base = addr & ~1
    md = md_t if thumb else md_a
    print(f"\n{'='*78}\n### {name}\n### vaddr {hex(base)}  ({'Thumb' if thumb else 'ARM'})\n{'='*78}")
    off = v2o(base)
    code = data[off:off + nbytes]
    hits = []
    for ins in md.disasm(code, base):
        op = ins.op_str
        mark = ""
        # a load through a register at #0x10 / #16 is the faulting shape
        if ins.mnemonic.startswith(("ldr", "ldm")) and re.search(r"#(0x10|16)\]", op):
            mark = "   <=== load at +0x10"
            hits.append(ins.address)
        # highlight the JNI vtable calls (JNIEnv is a table of fn ptrs)
        if ins.mnemonic == "blx" and ins.op_str.startswith("r"):
            mark = mark or "   (indirect call - JNIEnv-> )"
        print(f"  {ins.address:08x}  {ins.mnemonic:<8} {op}{mark}")
    if hits:
        print(f"  --> {len(hits)} load(s) at +0x10 in this function: " +
              ", ".join(hex(h) for h in hits))

for s in ("PvrClientJava11getLensInfo",
          "PvrClientJava14getDisplayInfo",
          "psmvr_UpdateLensAndDisplayInfoFromVRService",
          "_Z11GetLensInfoPN3PVR15_lensParametersE",
          "_Z19GetPvrServiceClientv"):
    dis(s)
print("\nDONE")

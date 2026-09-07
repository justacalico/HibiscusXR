#!/usr/bin/env python3
"""Read the numeric value of Pico's extra keycode labels from stock's libinput.so.

The KEYCODES table is an array of { const char* literal; int32_t value; }. On
AArch64 the pointer slot is filled by an R_AARCH64_RELATIVE relocation whose
addend is the string's vaddr, so the label -> value mapping has to be recovered
from .rela.dyn rather than read straight out of the file.
"""
import struct, sys

LIB = r"F:\PN2Lineage\notes\stock_system_lib64_libinput.so"
WANTED = [b"DEFINE_CONFIRM", b"DEFINE_CONTROLLER_CONFIRM", b"DC_IN",
          b"HALL_OPEN", b"HALL_CLOSE", b"ALL_APPS", b"BACK", b"HOME"]

data = open(LIB, "rb").read()
assert data[:4] == b"\x7fELF" and data[4] == 2, "expected 64-bit ELF"

e_shoff = struct.unpack_from("<Q", data, 0x28)[0]
e_shentsize = struct.unpack_from("<H", data, 0x3A)[0]
e_shnum = struct.unpack_from("<H", data, 0x3C)[0]
e_shstrndx = struct.unpack_from("<H", data, 0x3E)[0]

sections = []
for i in range(e_shnum):
    off = e_shoff + i * e_shentsize
    name, stype, flags, addr, offset, size, link, info, align, entsize = \
        struct.unpack_from("<IIQQQQIIQQ", data, off)
    sections.append(dict(name=name, type=stype, addr=addr, offset=offset,
                         size=size, entsize=entsize))

shstr = sections[e_shstrndx]
def sname(s):
    p = shstr["offset"] + s["name"]
    return data[p:data.index(b"\0", p)].decode()

for s in sections:
    s["sname"] = sname(s)

def vaddr_to_off(v):
    for s in sections:
        if s["addr"] and s["addr"] <= v < s["addr"] + s["size"]:
            return s["offset"] + (v - s["addr"])
    return None

# string vaddr for each wanted label
label_vaddr = {}
for s in sections:
    if s["sname"] not in (".rodata", ".rodata.str1.1"):
        continue
    blob = data[s["offset"]:s["offset"] + s["size"]]
    for w in WANTED:
        idx = 0
        while True:
            idx = blob.find(b"\0" + w + b"\0", idx)
            if idx < 0:
                break
            label_vaddr.setdefault(w, s["addr"] + idx + 1)
            break

# addend -> r_offset from .rela.dyn
rela = {}
for s in sections:
    if s["sname"].startswith(".rela"):
        n = s["size"] // 24
        for i in range(n):
            r_off, r_info, r_add = struct.unpack_from("<QQq", data, s["offset"] + i * 24)
            rela.setdefault(r_add, []).append(r_off)

print(f"{'label':<28} {'str vaddr':>12}  {'slot':>12}  value")
for w in WANTED:
    v = label_vaddr.get(w)
    if v is None:
        print(f"{w.decode():<28} {'not found':>12}")
        continue
    slots = rela.get(v, [])
    shown = False
    for slot in slots:
        off = vaddr_to_off(slot)
        if off is None:
            continue
        value = struct.unpack_from("<i", data, off + 8)[0]
        print(f"{w.decode():<28} {v:>#12x}  {slot:>#12x}  {value}")
        shown = True
    if not shown:
        print(f"{w.decode():<28} {v:>#12x}  {'no reloc':>12}")

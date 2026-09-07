#!/usr/bin/env python3
"""Teach our libinput.so the four Pico keycode labels the headset layouts need.

KeyLayoutMap::parseKey resolves each label through libinput's keycode table; one
unknown label fails the WHOLE file, so gpio-keys.kl was being discarded and every
headset button fell through Generic.kl - which is why Confirm arrived as ENTER.

Stock's values, read out of its own table:
    DC_IN 998, HALL_OPEN 999, HALL_CLOSE 1000, DEFINE_CONFIRM 1001

Rather than grow the array (fixed size, would move everything), repurpose entries
for keycodes this device can never generate. Each donor string is overwritten in
place, NUL padded to its original length, and the int32 beside it is replaced.
TV_* keycodes are meaningless on a VR headset.
"""
import struct, re, sys, shutil

# donor label -> (new label, new value)
PATCHES = {
    "TV_TIMER_PROGRAMMING": ("DEFINE_CONFIRM", 1001),
    "TV_TELETEXT":          ("HALL_OPEN",       999),
    "TV_SATELLITE_BS":      ("HALL_CLOSE",     1000),
    "TV_DATA_SERVICE":      ("DC_IN",           998),
}


def section_deltas(data):
    e_shoff = struct.unpack_from("<Q", data, 0x28)[0]
    e_shentsize = struct.unpack_from("<H", data, 0x3A)[0]
    e_shnum = struct.unpack_from("<H", data, 0x3C)[0]
    d = set()
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        _, _, _, addr, offset, size, _, _, _, _ = struct.unpack_from("<IIQQQQIIQQ", data, off)
        if addr and size:
            d.add(addr - offset)
    return d


def section_deltas32(data):
    e_shoff = struct.unpack_from("<I", data, 0x20)[0]
    e_shentsize = struct.unpack_from("<H", data, 0x2E)[0]
    e_shnum = struct.unpack_from("<H", data, 0x30)[0]
    d = set()
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        _, _, _, addr, offset, size, _, _, _, _ = struct.unpack_from("<IIIIIIIIII", data, off)
        if addr and size:
            d.add(addr - offset)
    return d


def patch(src, dst):
    shutil.copyfile(src, dst)
    data = bytearray(open(dst, "rb").read())
    is64 = data[4] == 2
    ptrsz = 8 if is64 else 4
    deltas = section_deltas(data) if is64 else section_deltas32(data)
    packer = (lambda v: struct.pack("<Q", v)) if is64 else (lambda v: struct.pack("<I", v))

    print(f"--- {dst}  ({'64' if is64 else '32'}-bit) ---")
    ok = 0
    for donor, (newlabel, newvalue) in PATCHES.items():
        i = data.find(b"\0" + donor.encode() + b"\0")
        if i < 0:
            print(f"  SKIP  donor {donor} not found"); continue
        stroff = i + 1
        if len(newlabel) > len(donor):
            print(f"  FAIL  {newlabel} longer than donor {donor}"); continue

        slots = []
        for d in deltas:
            slots += [m.start() for m in re.finditer(re.escape(packer(stroff + d)), data)]
        if len(slots) != 1:
            print(f"  FAIL  {donor}: expected 1 table slot, found {len(slots)}"); continue

        slot = slots[0]
        oldval = struct.unpack_from("<i", data, slot + ptrsz)[0]
        # overwrite string in place, NUL padded
        data[stroff:stroff + len(donor)] = newlabel.encode() + b"\0" * (len(donor) - len(newlabel))
        struct.pack_into("<i", data, slot + ptrsz, newvalue)
        print(f"  OK    {donor}({oldval}) -> {newlabel}({newvalue})  str={stroff:#x} slot={slot:#x}")
        ok += 1

    open(dst, "wb").write(data)
    print(f"  patched {ok}/{len(PATCHES)}")
    return ok == len(PATCHES)


a = patch(r"F:\PN2Lineage\notes\ourinput64.so", r"F:\PN2Lineage\notes\ourinput64.patched.so")
b = patch(r"F:\PN2Lineage\notes\ourinput32.so", r"F:\PN2Lineage\notes\ourinput32.patched.so")
sys.exit(0 if (a and b) else 1)

#!/usr/bin/env python3
import os
PN2_ROOT = os.environ.get("PN2_ROOT", os.path.expanduser("~/PN2Lineage"))
"""List our libinput's keycode label table so I can pick safe donor entries.

KeyLayoutMap::parseKey looks the label up in this table; an unknown label fails
the whole file. Pico's build has DEFINE_CONFIRM=1001 etc. appended; ours does not.
Rather than grow the array, repurpose entries for keycodes this headset can never
produce - overwriting the string in place (same length or shorter, NUL padded)
and the int beside it.
"""
import struct, re, sys

LIB = sys.argv[1] if len(sys.argv) > 1 else PN2_ROOT + "/notes/ourinput64.so"
data = open(LIB, "rb").read()

# find .rodata so we can convert file offset -> vaddr
e_shoff = struct.unpack_from("<Q", data, 0x28)[0]
e_shentsize = struct.unpack_from("<H", data, 0x3A)[0]
e_shnum = struct.unpack_from("<H", data, 0x3C)[0]
deltas = set()
for i in range(e_shnum):
    off = e_shoff + i * e_shentsize
    _, _, _, addr, offset, size, _, _, _, _ = struct.unpack_from("<IIQQQQIIQQ", data, off)
    if addr and size:
        deltas.add(addr - offset)

def slots_for(stroff):
    out = []
    for d in deltas:
        needle = struct.pack("<Q", stroff + d)
        out += [m.start() for m in re.finditer(re.escape(needle), data)]
    return out

# candidate labels: long enough to hold DEFINE_CONFIRM (14) and irrelevant to a VR headset
CANDIDATES = ["MEDIA_AUDIO_TRACK", "NAVIGATE_PREVIOUS", "SYSTEM_NAVIGATION_UP",
              "SYSTEM_NAVIGATION_DOWN", "MEDIA_SKIP_FORWARD", "MEDIA_SKIP_BACKWARD",
              "MEDIA_STEP_FORWARD", "MEDIA_STEP_BACKWARD", "TV_SATELLITE_BS",
              "TV_TELETEXT", "TV_TIMER_PROGRAMMING", "TV_CONTENTS_MENU",
              "TV_MEDIA_CONTEXT_MENU", "TV_DATA_SERVICE", "TV_RADIO_SERVICE",
              "ALL_APPS", "SOFT_SLEEP", "CUT", "COPY", "PASTE"]

print(f"{'label':<26} {'len':>4} {'stroff':>9} {'slots':>6}  value")
for w in CANDIDATES:
    i = data.find(b"\0" + w.encode() + b"\0")
    if i < 0:
        print(f"{w:<26} {'-':>4} {'not found':>9}")
        continue
    stroff = i + 1
    s = slots_for(stroff)
    val = struct.unpack_from("<i", data, s[0] + 8)[0] if s else None
    print(f"{w:<26} {len(w):>4} {stroff:>#9x} {len(s):>6}  {val}")

print()
print("need >= 14 chars to hold DEFINE_CONFIRM, >= 25 for DEFINE_CONTROLLER_CONFIRM")

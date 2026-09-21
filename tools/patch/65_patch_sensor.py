#!/usr/bin/env python3
"""
Neutralise the fatal sensor-type CHECK in libsensorservice.so (arm64).

Android 10's convertToSensorEvent() does:

    if (sensorType <= 35)            -> handle known standard types
    else if (sensorType < 65536)     -> LOG(FATAL): "Check failed:
                                        sensorType >= DEVICE_PRIVATE_BASE"
    else                             -> copy the event payload verbatim

Pico's HAL emits types 57, 58, 126 and 127. Android 8.1 never checked, so the
stock OS was fine; Android 10 kills system_server every time one arrives, which
reboots the device. Observed: 14 restarts in 63s when a fused sensor is enabled,
and again the moment the camera app touches the tracking cameras.

The whole check is one conditional branch:

    2bea8: cmp   w19, #16, lsl #12      ; compare with 65536
    2beac: b.lt  0x2bef8                ; below -> abort path
    2beb0: ldp   q1, q0, [x0, #48]      ; fall-through: copy payload, return

Replacing the b.lt with a NOP makes every out-of-range type take the payload-copy
path, i.e. get treated as a private sensor. That is what the stock 8.1 behaviour
effectively was, and no one is listening to those sensors anyway.

Deliberately a single 4-byte edit, not a rewrite: the smaller the change, the
easier it is to argue it is correct.
"""
import shutil
import sys

OFFSET   = 0x2BEAC
EXPECT   = bytes([0x6B, 0x02, 0x00, 0x54])   # b.lt #0x4c
NOP      = bytes([0x1F, 0x20, 0x03, 0xD5])   # nop

# the guard instruction immediately before, as a sanity anchor
ANCHOR_OFF = 0x2BEA8
ANCHOR     = bytes([0x7F, 0x42, 0x40, 0x71])  # cmp w19, #16, lsl #12

# ---------------------------------------------------------------- patch 2
#
# Second fatal CHECK, same function, the DYNAMIC_SENSOR_META case:
#   'Check failed: it != mConnectedDynamicSensors.end()'
#
# Pico reports a dynamic-sensor CONNECT for a handle that sensorservice never
# registered, so the inlined unordered_map::find misses and Q aborts. Opening
# ALVR triggers it and reboots the device.
#
#   17c5c: cbz w8, 0x17d58        ; if (!connected) skip the whole block
#   17c60..17cec: find(handle)
#   17cf0: <LOG(FATAL) block>     ; every miss lands here
#   17d48: dst->...sensor = it->second; memcpy(uuid, ...)
#   17d58: <common continuation>
#
# Redirect the miss to 0x17d58 rather than NOPing: falling through would run the
# 0x17d48 code with an invalid iterator. 0x17d58 is already the target the
# function uses for the !connected case, so "skip filling in sensor/uuid and
# carry on" is behaviour this code already implements.
#
#   b 0x17d58  from 0x17cf0  ->  (0x17d58-0x17cf0)/4 = 26 = 0x1a
#   encoding 0x14000000 | 26 = 0x1400001a
# FIRST ATTEMPT WAS WRONG, recorded here so it is not repeated: branching the
# miss straight to 0x17d58 skipped the abort but left
# dst->dynamic_sensor_meta.connected == true with .sensor never assigned.
# SensorService::threadLoop() then did
#     const sensor_t& s = *(event.dynamic_sensor_meta.sensor);
# on a null pointer and system_server segfaulted at threadLoop()+1660 with
# fault addr 0x18 - the same crash loop, just harder to read than the abort.
#
# threadLoop only dereferences .sensor when .connected is true; the else branch
# removes by handle and touches nothing. So the miss path has to CLEAR connected
# and then continue. The dead abort block leaves 88 bytes to write into:
#
#   17cf0:  mov  w8, #0
#   17cf4:  str  w8, [x19, #24]     ; dst->dynamic_sensor_meta.connected = 0
#   17cf8:  b    0x17d58            ; common continuation
#
# sensors_event_t layout confirmed from the surrounding code:
#   +24 connected, +28 handle, +32 sensor*, +40 uuid[16]
OFFSET2  = 0x17CF0
EXPECT2  = bytes([0x81, 0xFF, 0xFF, 0xF0])   # adrp x1, 0xa000
BRANCH2  = bytes([
    0x08, 0x00, 0x80, 0x52,   # mov w8, #0
    0x68, 0x1A, 0x00, 0xB9,   # str w8, [x19, #24]
    0x18, 0x00, 0x00, 0x14,   # b 0x17d58   ((0x17d58-0x17cf8)/4 = 24)
])
# the earlier, broken version - detected so a half-patched file is not mistaken
# for an unpatched one
BRANCH2_BAD = bytes([0x1A, 0x00, 0x00, 0x14])

# anchor: the two adds that build the assert strings, right after
ANCHOR2_OFF = 0x17CF8
ANCHOR2     = bytes([0x21, 0x50, 0x08, 0x91])  # add x1, x1, #532


def main(src, dst):
    data = bytearray(open(src, "rb").read())
    print(f"input : {src} ({len(data)} bytes)")

    anchor = bytes(data[ANCHOR_OFF:ANCHOR_OFF + 4])
    if anchor != ANCHOR:
        print(f"ABORT: anchor mismatch at 0x{ANCHOR_OFF:x}: "
              f"got {anchor.hex()} want {ANCHOR.hex()}")
        return 1
    print(f"anchor  0x{ANCHOR_OFF:x}: {anchor.hex()}  (cmp w19, #65536) OK")

    cur = bytes(data[OFFSET:OFFSET + 4])
    if cur == NOP:
        # do NOT return here: patch 2 still has to be considered
        print("patch1 already applied")
    elif cur != EXPECT:
        print(f"ABORT: unexpected bytes at 0x{OFFSET:x}: "
              f"got {cur.hex()} want {EXPECT.hex()}")
        return 1

    if cur == EXPECT:
        data[OFFSET:OFFSET + 4] = NOP
        print(f"patched 0x{OFFSET:x}: {cur.hex()} -> {NOP.hex()}  (b.lt -> nop)")

    # --- patch 2: DYNAMIC_SENSOR_META map-miss ---------------------------
    a2 = bytes(data[ANCHOR2_OFF:ANCHOR2_OFF + 4])
    if a2 != ANCHOR2:
        print(f"ABORT: patch2 anchor mismatch at 0x{ANCHOR2_OFF:x}: "
              f"got {a2.hex()} want {ANCHOR2.hex()}")
        return 1
    print(f"anchor  0x{ANCHOR2_OFF:x}: {a2.hex()}  (add x1, x1, #532) OK")

    cur2 = bytes(data[OFFSET2:OFFSET2 + len(BRANCH2)])
    head2 = bytes(data[OFFSET2:OFFSET2 + 4])
    if cur2 == BRANCH2:
        print("patch2 already applied")
    elif head2 == BRANCH2_BAD:
        # overwrite the earlier broken version in place
        data[OFFSET2:OFFSET2 + len(BRANCH2)] = BRANCH2
        print(f"patched 0x{OFFSET2:x}: replaced the BROKEN skip-only patch "
              f"with clear-connected + branch")
    elif head2 != EXPECT2:
        print(f"ABORT: unexpected bytes at 0x{OFFSET2:x}: "
              f"got {head2.hex()} want {EXPECT2.hex()}")
        return 1
    else:
        data[OFFSET2:OFFSET2 + len(BRANCH2)] = BRANCH2
        print(f"patched 0x{OFFSET2:x}: abort block -> "
              f"mov w8,#0 / str w8,[x19,#24] / b 0x17d58")

    open(dst, "wb").write(data)
    print(f"output: {dst} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))

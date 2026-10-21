#!/bin/bash
HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
# Add the QVR client libraries to system-pn2-full.img.
#
# Same Treble trap as libcdsprpc.so, and the one that finally unblocked tracking:
# libqvrservice_client.so ships only in /vendor/lib{,64}. pvrservice runs from
# /system/bin, and Android 10 forbids a /system process loading from /vendor/lib.
# Android 8.1 allowed it, which is why stock works. The dlopen failed silently,
# QVRServiceClient_Create returned NULL, and the SVR layer concluded
# "QVR Serivce reported VR not supported" - so 6DoF never started, the SDK's
# cached trackingstate stayed 0, and every pose was rejected.
#
# With these in /system: "QVR Service supports positional tracking",
# BadPose=0, kLostDialog=0, trackingstate!=0.
#
# libqvrcamera_client.so is the same story and is what airservice/see-through need.
set -u
IMG=${HIBISCUS_ROOT}/out/system-pn2-full.img
SRC=${HIBISCUS_ROOT}/qvr
LOG=${HIBISCUS_ROOT}/notes/352_img_qvrclient.txt
exec >"$LOG" 2>&1

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo "=== free before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

echo
echo "=== QVR client libraries (from /vendor, both ABIs) ==="
put "$SRC/lib64/libqvrservice_client.so" /lib64/libqvrservice_client.so 644
put "$SRC/lib/libqvrservice_client.so"   /lib/libqvrservice_client.so   644
put "$SRC/lib64/libqvrcamera_client.so"  /lib64/libqvrcamera_client.so  644
put "$SRC/lib/libqvrcamera_client.so"    /lib/libqvrcamera_client.so    644

echo
echo "=== repair + verify ==="
img_fsck || fail=$((fail+1))

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "QVR CLIENT ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

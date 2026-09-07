#!/bin/bash
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
IMG=/mnt/f/PN2Lineage/out/system-pn2-full.img
SRC=/mnt/f/PN2Lineage/qvr
LOG=/mnt/f/PN2Lineage/notes/352_img_qvrclient.txt
exec >"$LOG" 2>&1

fail=0
put() {
  local src="$1" dst="$2" mode="$3" dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  [ -f "$src" ] || { printf '  MISSING SRC %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then printf '  OK    %-46s %12s\n' "$dst" "$got"
  else printf '  FAIL  %-46s want %s got "%s"\n' "$dst" "$want" "${got:-absent}"; fail=$((fail+1)); fi
}

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
e2fsck -fy "$IMG" 2>&1 | tail -4
if e2fsck -fn "$IMG" >/tmp/f2.txt 2>&1; then tail -2 /tmp/f2.txt; echo "  CLEAN"; else tail -6 /tmp/f2.txt; echo "  DIRTY"; fail=$((fail+1)); fi

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "QVR CLIENT ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

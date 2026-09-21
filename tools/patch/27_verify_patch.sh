#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
PW="$1"
R=${PN2_ROOT}/gsi/gsi_raw.img
M="${GSI_MNT:-/mnt/gsi_edit}"
L=${PN2_ROOT}/notes/27_verify_patch.log
exec >"$L" 2>&1

echo "$PW" | sudo -S mount -o loop,ro "$R" "$M" 2>/dev/null
if ! mountpoint -q "$M"; then echo "ABORT: mount failed"; exit 1; fi

echo "=== tail of /build.prop ==="
echo "$PW" | sudo -S tail -12 "$M/build.prop"
echo
echo "=== the keys we care about ==="
echo "$PW" | sudo -S grep -nE '^(persist\.sys\.usb\.config|ro\.adb\.secure|ro\.debuggable|ro\.secure)=' "$M/build.prop"
echo
if [ -f "$M/etc/prop.default" ]; then
  echo "=== /etc/prop.default keys ==="
  echo "$PW" | sudo -S grep -nE '^(persist\.sys\.usb\.config|ro\.adb\.secure|ro\.debuggable|ro\.secure)=' "$M/etc/prop.default"
else
  echo "(no /etc/prop.default)"
fi
echo
echo "$PW" | sudo -S umount "$M"
echo DONE

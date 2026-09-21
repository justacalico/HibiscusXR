#!/system/bin/sh
echo "=== pvrservice alive? ==="
ps -A 2>/dev/null | grep -iE 'pvrservice|airservice|qvrservice'
echo "init.svc.pvrservice = $(getprop init.svc.pvrservice)"
echo
echo "=== is it registered with servicemanager? ==="
service list 2>/dev/null | grep -iE 'pvrservice|pvr_manager|air'
echo
echo "=== pvrservice recent log ==="
logcat -d -t 400 2>/dev/null | grep -iE 'PvrService|pvrservice' | tail -20
echo
echo "=== did pvrservice crash? ==="
T=$(ls -t /data/tombstones/tombstone_* 2>/dev/null | head -3)
for f in $T; do echo "--- $f"; grep -m1 -E '^pid:|Timestamp' "$f"; done

#!/system/bin/sh
echo "=== identity ==="
getprop ro.product.device
getprop ro.build.version.release
getprop ro.pvr.internal.version
getprop sys.boot_completed
echo "uptime: $(cut -d. -f1 /proc/uptime)s"
echo

echo "=== pstore contents ==="
ls -la /sys/fs/pstore/ 2>&1
echo
echo "=== last_kmsg (older path) ==="
ls -la /proc/last_kmsg 2>&1
echo

echo "=== mounts sanity ==="
grep -E " /system | /vendor | /persist | /oem " /proc/mounts
echo

echo "=== persist calibration still intact? ==="
ls -l /persist/pvr/camera/ 2>&1
ls -l /persist/ndi/ 2>&1 | head -6
echo

echo "=== VR stack running on stock? ==="
getprop init.svc.pvrservice
getprop init.svc.qvrservice
ps -A 2>/dev/null | grep -iE 'pvr|qvr' | head -5

#!/system/bin/sh
# The tracking-lost dialog is back and the controller service is running again
# despite pm disable. Find out what reset and whether pvrservice is wedged again.
echo "=== uptime (did it reboot since my tests?) ==="
uptime
cat /proc/uptime | awk '{printf "  up %.0f min\n", $1/60}'
echo
echo "=== is the controller package really disabled? ==="
pm list packages -e | grep -c cvcontroller
echo "  ^ 1 = ENABLED again, 0 = still disabled"
ps -A | grep -i cvcontroller | awk '{print "  running: "$1" "$NF}'
echo
echo "=== is pvrservice wedged again? ==="
P=$(pidof pvrservice)
echo "  pid $P"
echo "  malloc/fork contended threads: $(debuggerd -b $P 2>&1 | grep -c 'je_malloc_mutex\|atfork')"
echo "  zombie children: $(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P '$2==p && $3 ~ /Z/' | wc -l)"
echo
echo "=== does QVR still report positional tracking? ==="
logcat -d | grep -iE "QVR Serivce reported|QVR Service supports" | tail -4
echo "  (empty = rotated out of the buffer, not necessarily bad)"
echo
echo "=== are the qvr client libs still in /system? ==="
ls -l /system/lib64/libqvrservice_client.so /system/lib/libqvrservice_client.so 2>/dev/null || echo "  GONE - /system was remounted ro or reflashed"
echo
echo "=== which VRShell SDK is installed (x27 patch should be 120a6620...) ==="
md5sum /system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so
echo
echo "=== live tracking state ==="
logcat -d | grep -iE "trackingstate|Bad Pose|kLostDialog|CalculateDialogState" | tail -6

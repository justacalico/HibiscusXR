#!/system/bin/sh
# The shell renders but sits at "loading". Find what it is waiting for.
V=$(pidof com.pvr.vrshell)
echo "=== vrshell pid $V, threads $(ls /proc/$V/task 2>/dev/null | wc -l) ==="
echo
echo "=== its last 30 log lines ==="
logcat -d 2>/dev/null | grep " $V " | grep -viE 'Filename|^\s*$' | tail -30
echo
echo "=== is it blocked on a service? ==="
logcat -d 2>/dev/null | grep -iE 'Waiting for service|didn.t start|binderdied|not published|timeout|ANR' | tail -10
echo
echo "=== compositor sample ==="
logcat -c
sleep 8
echo "  BadPose  = $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  NoEyeBuf = $(logcat -d 2>/dev/null | grep -c 'No valid Eye')"
echo "  ts0      = $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0')"
echo "  unity    = $(logcat -d 2>/dev/null | grep -c 'Unity   :')"
echo
echo "=== last unity lines ==="
logcat -d 2>/dev/null | grep -iE 'Unity   :|PVRShell' | grep -viE 'Filename' | tail -10

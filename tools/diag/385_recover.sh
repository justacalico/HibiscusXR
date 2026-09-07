#!/system/bin/sh
# Get back to a known-good state: shell enabled, one VR client, clean services.
echo "=== what is enabled / running ==="
echo "  vrshell disabled?  $(pm list packages -d 2>/dev/null | grep -c pvr.vrshell)"
echo "  seethrough disabled? $(pm list packages -d 2>/dev/null | grep -c seethrough)"
echo "  vrshell pid    [$(pidof com.pvr.vrshell)]"
echo "  seethrough pid [$(pidof com.pvr.seethrough.setting)]"
echo "  pvrservice pid [$(pidof pvrservice)]"
echo "  airservice pid [$(pidof airservice)]"

echo
echo "=== re-enable anything I parked ==="
pm enable com.pvr.vrshell >/dev/null 2>&1
pm enable com.pvr.seethrough.setting >/dev/null 2>&1
setprop persist.pvrcon.seethrough.enable 0

echo
echo "=== one VR client only: stop see-through, keep the shell ==="
am force-stop com.pvr.seethrough.setting
sleep 2

echo
echo "=== clean restart of the stack ==="
stop pvrservice; sleep 3; start pvrservice; sleep 6
stop airservice; sleep 2; start airservice; sleep 4
echo "  pvrservice [$(pidof pvrservice)]  airservice [$(pidof airservice)]"

echo
echo "=== bring the shell up ==="
logcat -c
am start -n com.pvr.vrshell/.MainActivity 2>&1 | head -2
sleep 30

V=$(pidof com.pvr.vrshell)
echo "  vrshell pid [$V] threads=$(ls /proc/$V/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== health ==="
echo "  segv=$(logcat -d | grep -c 'exited due to signal 11')"
echo "  shmem failures=$(logcat -d | grep -c 'get share memory fd failed')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"
logcat -d | grep -i 'getTrackingDataExt position' | tail -2
echo "  fan rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null) state=$(cat /sys/class/thermal/cooling_device1/cur_state 2>/dev/null)"

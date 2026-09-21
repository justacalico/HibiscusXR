#!/system/bin/sh
# Experiment done - see-through cannot complete until its own copy of the Pico SDK
# gets the x28 patch. Give the headset its home back.
echo "=== re-enable the shell ==="
pm enable com.pvr.vrshell 2>&1 | tail -1
am force-stop com.pvr.seethrough.setting
sleep 2

echo
echo "=== clean services ==="
stop pvrservice; sleep 3; start pvrservice; sleep 6
stop airservice; sleep 2; start airservice; sleep 4
echo "  pvrservice [$(pidof pvrservice)]  airservice [$(pidof airservice)]"

logcat -c
echo
echo "=== launch the shell ==="
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
echo "  hottest cpu=$(cat /sys/class/thermal/thermal_zone12/temp 2>/dev/null | cut -c1-2) C"

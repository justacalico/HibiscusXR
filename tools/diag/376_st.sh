#!/system/bin/sh
# Launch the see-through app with the BitTube guard in place, the controller
# service enabled and the fan running. Deliberately NOT setting
# persist.pvrcon.seethrough.enable=1 - that makes the system relaunch the flow
# forever and bootloops the 2D loading screen.
logcat -c
am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -3
sleep 30

S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
echo "  vrshell pid    [$(pidof com.pvr.vrshell)]"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== crashes ==="
n=$(logcat -d | grep -c 'exited due to signal 11')
echo "  segv count: $n"
logcat -d | grep -E '>>> .* <<<' | tail -3

echo
echo "=== did it enter VR mode / open the see-through cameras? ==="
logcat -d | grep -iE 'EnterVrMode|TimeWarp|AIRService|aircamera|SeeThrough|StartCameraInternal' | grep -v LockBuffer | tail -14

echo
echo "=== app log tail ==="
logcat -d | grep " $S " | grep -viE 'chatty|Undefined variable|avc:|Override displayinfo' | tail -22

echo
echo "=== tracking ==="
echo "  BadPose=$(logcat -d | grep -c 'Bad Pose')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)"
logcat -d | grep -i 'getTrackingDataExt position' | tail -2

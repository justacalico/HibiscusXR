echo "=== what is still crashing? ==="
logcat -d | grep -A8 "backtrace:" | grep -E "#0[0-4] " | head -10
echo "  --- and which process ---"
logcat -d | grep -E ">>> .* <<<" | tail -3
echo
echo "=== launch see-through ==="
logcat -c
am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -3
sleep 30
S=$(pidof com.pvr.seethrough.setting)
echo "  pid $S  threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m1 mCurrentFocus
echo
echo "=== did it reach VR mode / open the cameras? ==="
logcat -d | grep -iE "EnterVrMode|TimeWarp|aircamera|AIRService|SeeThrough|StartCameraInternal" | grep -viE "LockBuffer" | tail -12
echo
echo "=== app log tail ==="
logcat -d | grep " $S " | grep -viE "chatty|Undefined variable|avc:|Override displayinfo" | tail -22
echo
echo "=== tracking still alive? ==="
echo "  kLost=$(logcat -d | grep -c kLostDialog)"
logcat -d | grep -iE "getTrackingDataExt position" | tail -2

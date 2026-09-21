logcat -c
am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -3
sleep 22
P=$(pidof com.pvr.seethrough.setting)
echo "  pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"
echo "--- focus ---"
dumpsys window | grep mCurrentFocus
echo "--- crashes ---"
logcat -d | grep -E "E CRASH|signal 11|FATAL|does not exist" | head -6
echo "  (empty above = no crash)"
echo "--- did it enter VR / open cameras? ---"
logcat -d | grep -iE "EnterVrMode|TimeWarp|aircamera|airservice|SeeThrough|camera.*open|QVRServiceCamDeviceHAL3" | grep -viE "LockBuffer" | tail -12
echo "--- app log tail ---"
logcat -d | grep " $P " | grep -viE "chatty|Undefined variable|avc:|Override displayinfo" | tail -25

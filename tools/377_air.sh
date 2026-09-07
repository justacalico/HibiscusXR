#!/system/bin/sh
# I stopped airservice while shedding thermal load and never restarted it. Both
# VRShell and the see-through app block waiting for it to publish, and the
# see-through camera feed is exactly what it provides.
echo "=== start airservice ==="
start airservice
sleep 8
echo "  pid $(pidof airservice)"
echo "  init.svc.airservice = [$(getprop init.svc.airservice)]"
echo "  published: $(service check airservice 2>/dev/null)"

echo
echo "=== relaunch see-through on top of it ==="
logcat -c
am force-stop com.pvr.seethrough.setting
sleep 2
am start -n com.pvr.seethrough.setting/.MainActivity >/dev/null 2>&1
sleep 30

S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== airservice / camera ==="
logcat -d | grep -iE 'AIRService|AIRClient|aircamera|QVRServiceCamDeviceHAL3' | grep -v LockBuffer | tail -14

echo
echo "=== share memory + tracking ==="
logcat -d | grep -iE 'share memory|getShareMemoryFD' | tail -4
echo "  BadPose=$(logcat -d | grep -c 'Bad Pose')  kLost=$(logcat -d | grep -c kLostDialog)  segv=$(logcat -d | grep -c 'exited due to signal 11')"

echo
echo "=== did it reach VR / draw ==="
logcat -d | grep -iE 'EnterVrMode|TimeWarp|Nothing to draw' | tail -8

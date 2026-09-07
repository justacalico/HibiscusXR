#!/system/bin/sh
# The see-through app cannot get the tracking share-memory fd while VRShell is
# running and holding it. pvrservice's newTracker share memory looks single-client,
# the same way QVR allows exactly one VR-mode owner. Give see-through the field
# to itself.
echo "=== stop the shell so see-through is the only VR client ==="
am force-stop com.pvr.vrshell
am force-stop com.pvr.seethrough.setting
sleep 3
echo "  vrshell pid [$(pidof com.pvr.vrshell)]"

echo
echo "=== fresh pvrservice + airservice ==="
stop pvrservice; sleep 3; start pvrservice; sleep 6
stop airservice; sleep 2; start airservice; sleep 6
echo "  pvrservice [$(pidof pvrservice)]  airservice [$(pidof airservice)]"

logcat -c
echo
echo "=== launch see-through alone ==="
am start -n com.pvr.seethrough.setting/.MainActivity >/dev/null 2>&1
sleep 30

S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"

echo
echo "=== share memory now? ==="
echo "  failures: $(logcat -d | grep -c 'get share memory fd failed')"
logcat -d | grep -iE 'share memory|ShareMemory' | tail -6

echo
echo "=== tracking in the app ==="
echo "  GetTrackingData failures: $(logcat -d | grep -c 'GetTrackingData 2 failed')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"

echo
echo "=== camera / passthrough ==="
logcat -d | grep -iE 'aircamera|AIRService|processCameraStateChange|CamDeviceHAL3' | grep -v LockBuffer | tail -12

echo
echo "=== shim stubs called yet? ==="
logcat -d | grep -i shim_air | tail -3
echo "  (empty = still not on the path)"

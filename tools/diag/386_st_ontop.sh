#!/system/bin/sh
# Launch see-through on top of a shell that is already tracking. No force-stops,
# no service restarts - leave the working state alone and just observe.
logcat -c
am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -2
sleep 30

echo
echo "  vrshell pid    [$(pidof com.pvr.vrshell)]"
S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== camera frames ==="
echo "  get6DofImage=$(logcat -d | grep -c get6DofImage)  cameraStarted=$(logcat -d | grep -c QVRCAMERA_CAMERA_STARTED)  closed=$(logcat -d | grep -c closeQvrCamera)"
logcat -d | grep -iE 'get6DofImage|QVRCAMERA_CAMERA|stopPreview|closeQvrCamera|startPreview|processCameraStateChange' | tail -10

echo
echo "=== share memory / tracking ==="
echo "  shmem failures=$(logcat -d | grep -c 'get share memory fd failed')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"
logcat -d | grep -i 'getTrackingDataExt position' | tail -2

echo
echo "=== boundary algorithm ==="
logcat -d | grep -iE 'startAlgorithm|loadSymbols|SafetyAreaRecovery' | tail -4
ls -l /data/misc/user/0/boundary/stdata.txt

echo
echo "=== errors from the app ==="
logcat -d | grep " $S " | grep -E ' [EF] ' | grep -viE 'Undefined variable|Override displayinfo' | tail -12

#!/system/bin/sh
# VRShell is home, so force-stop just respawns it and it immediately takes the
# tracking share memory and the QVR camera back. Both are single-client. Disable
# it for the duration so the see-through app owns the stack outright.
echo "=== park VRShell ==="
pm disable com.pvr.vrshell 2>&1 | tail -1
am force-stop com.pvr.vrshell
sleep 3
echo "  vrshell pid [$(pidof com.pvr.vrshell)]"

echo
echo "=== fresh services ==="
stop pvrservice; sleep 3; start pvrservice; sleep 6
stop airservice; sleep 2; start airservice; sleep 6
echo "  pvrservice [$(pidof pvrservice)]  airservice [$(pidof airservice)]"

logcat -c
echo
echo "=== see-through alone ==="
am start -n com.pvr.seethrough.setting/.MainActivity >/dev/null 2>&1
sleep 35

S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== camera frames ==="
echo "  get6DofImage count: $(logcat -d | grep -c get6DofImage)"
echo "  camera stopped   : $(logcat -d | grep -c closeQvrCamera)"
logcat -d | grep -iE 'get6DofImage|QVRCAMERA_CAMERA|stopPreview|closeQvrCamera|startPreview' | tail -10

echo
echo "=== tracking + share memory ==="
echo "  shmem failures=$(logcat -d | grep -c 'get share memory fd failed')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"

echo
echo "=== algorithm ==="
logcat -d | grep -iE 'startAlgorithm|loadSymbols|SafetyAreaRecovery' | tail -5
ls -l /data/misc/user/0/boundary/stdata.txt

echo
echo "=== shim stubs ==="
logcat -d | grep -i shim_air | tail -3
echo "  (empty = not on the path)"

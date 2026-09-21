#!/system/bin/sh
# Park VRShell and give the see-through calibration the whole stack, the way stock
# sequences it at first boot. The tracking share memory and the QVR camera are both
# single-client, and VRShell is home so force-stop alone just respawns it.
#
# Recovery is one command if anything goes wrong:
#     pm enable com.pvr.vrshell
set -u

echo "=== park VRShell ==="
pm disable com.pvr.vrshell 2>&1 | tail -1
am force-stop com.pvr.vrshell
am force-stop com.pvr.seethrough.setting
sleep 3
echo "  vrshell pid [$(pidof com.pvr.vrshell)]  (empty = parked)"

echo
echo "=== clean services so nothing holds the camera ==="
stop pvrservice; sleep 3; start pvrservice; sleep 6
stop airservice; sleep 2; start airservice; sleep 6
echo "  pvrservice [$(pidof pvrservice)]  airservice [$(pidof airservice)]"

echo
echo "=== boundary data before ==="
ls -l /data/misc/user/0/boundary/stdata.txt /data/misc/user/0/boundary/stdataforalgorithm.txt

logcat -c
echo
echo "=== launch see-through with the field to itself ==="
am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -2

i=0
while [ $i -lt 6 ]; do
    sleep 15
    S=$(pidof com.pvr.seethrough.setting)
    F=$(logcat -d | grep -c get6DofImage)
    C=$(logcat -d | grep -c closeQvrCamera)
    SZ=$(stat -c%s /data/misc/user/0/boundary/stdata.txt 2>/dev/null)
    echo "  t+$(( (i+1)*15 ))s  pid=[$S]  frames=$F  cameraClosed=$C  stdata=${SZ}B"
    i=$((i+1))
done

echo
echo "=== camera path ==="
logcat -d | grep -iE 'get6DofImage|QVRCAMERA_CAMERA|startPreview|stopPreview|closeQvrCamera' | tail -8

echo
echo "=== tracking ==="
echo "  shmem failures=$(logcat -d | grep -c 'get share memory fd failed')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"

echo
echo "=== algorithm ==="
logcat -d | grep -iE 'startAlgorithm|loadSymbols|SafetyArea' | tail -4

echo
echo "=== focus ==="
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus
echo
echo "VRShell is parked. Re-enable with:  pm enable com.pvr.vrshell"

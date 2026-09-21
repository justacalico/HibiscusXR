#!/system/bin/sh
# Pose is accepted now ("No valid Eye Buffers" instead of "Bad Pose"), which is the
# same state stock shows at startup. The app just has not submitted eye buffers.
# Is Unity actually rendering?
V=$(pidof com.pvr.vrshell)
echo "=== vrshell pid $V, threads $(ls /proc/$V/task 2>/dev/null | wc -l) ==="
echo
echo "=== is Unity running its scene? ==="
logcat -d 2>/dev/null | grep -iE 'Unity   :' | tail -12
echo
echo "=== eye buffer / render events ==="
logcat -d 2>/dev/null | grep -iE 'EyeBuffer|CameraEndFrame|BeginEye|EndEye|TimeWarpEvent|SetEyeBufferSize|thisEyeBufferNum' | tail -10
echo
echo "=== current compositor state ==="
logcat -d -t 60 2>/dev/null | grep -iE 'SelectRT' | tail -4
echo
echo "=== trackingstate now ==="
logcat -d -t 200 2>/dev/null | grep -c 'trackingstate = 0x0,0x0'

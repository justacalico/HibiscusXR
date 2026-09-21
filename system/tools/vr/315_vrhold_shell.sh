#!/system/bin/sh
# Proven earlier: with QVR VR mode held open the cameras open (QVRServiceCamDeviceHAL3)
# and the DSP tracker emits real positional data. Nothing holds it in normal
# operation, so hold it deliberately and start the shell behind it.
#
# qvrservicetest -d takes a duration in seconds; use a long one as a stand-in for
# the holder that pvrservice/CVService provide on stock.
pkill -f qvrservicetest 2>/dev/null
sleep 1
echo "=== holding VR mode (tracking mode 3, 1 hour) ==="
/vendor/bin/qvrservicetest64 -t 3 -d 3600 >/dev/null 2>&1 &
sleep 8
logcat -d 2>/dev/null | grep -iE 'VR Mode started|CamDeviceHAL3' | tail -4

echo
echo "=== restarting the shell behind it ==="
am force-stop com.pvr.vrshell
am force-stop com.pvr.seethrough.setting
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 22

echo
echo "=== compositor (20s of shell running with VR mode held) ==="
echo "  Bad Pose        : $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  No valid EyeBuf : $(logcat -d 2>/dev/null | grep -c 'No valid Eye Buffers')"
echo "  trackingstate 0 : $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0,0x0')"
echo
echo "=== tracking data - is position non-zero (6DoF)? ==="
logcat -d 2>/dev/null | grep -iE 'getTrackingDataExt position|svrHeadPoseStateOutput' | tail -4
echo
echo "=== cameras ==="
logcat -d 2>/dev/null | grep -iE 'CamDeviceHAL3|CAMERA_READY|openCamera' | tail -5

#!/system/bin/sh
echo "=========== QVRServiceClient C API (this is the whole question) ==========="
strings -a /vendor/lib64/libqvrservice_client.so | grep -E "^QVRServiceClient_" | sort -u

echo
echo "=========== QVRCameraClient / QVRCameraDevice C API ==========="
strings -a /vendor/lib64/libqvrcamera_client.so | grep -E "^QVRCamera(Client|Device)_" | sort -u

echo
echo "=========== who is qvrservice's client right now ==========="
ls -l /proc/1396/exe 2>/dev/null
cat /proc/1396/cmdline 2>/dev/null; echo
echo "--- pvrservice is the current VR-mode owner? ---"
getprop | grep -iE "qvr|vr.mode|persist.vr" | head -20

echo
echo "=========== camera intrinsics/extrinsics we already have ==========="
cat /persist/pvr/camera/device_calibration.xml 2>/dev/null | head -60

echo
echo "=========== lens axis offset ==========="
cat /persist/pvr/lens/axisOffset.txt 2>/dev/null

echo
echo "=========== distortion / vrapi config ==========="
ls -l /system/etc/pvr/ 2>/dev/null
head -40 /system/etc/pvr/psmvrapi_config1.txt 2>/dev/null

echo
echo "=========== real IMU rate available ==========="
dumpsys sensorservice 2>/dev/null | grep -iE "gyro|accel" | grep -iE "minRate|maxRate|type:" | head -20

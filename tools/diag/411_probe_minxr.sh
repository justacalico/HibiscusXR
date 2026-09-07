#!/system/bin/sh
# What would a bare OpenXR image actually need from the vendor side?
echo "=========== 1. vendor daemons currently running ==========="
ps -A -o PID,USER,NAME 2>/dev/null | grep -iE "qvr|pvr|air|sensor|cdsp|adsp|slpi|camera|vendor" | grep -v grep

echo
echo "=========== 2. what vendor VR binaries exist at all ==========="
ls -l /vendor/bin/ 2>/dev/null | grep -iE "qvr|pvr|vr|sensor|camera"

echo
echo "=========== 3. IMU: does Android's sensor HAL even see the VR gyro ==========="
dumpsys sensorservice 2>/dev/null | sed -n '1,40p'

echo
echo "=========== 4. display topology + refresh ==========="
dumpsys SurfaceFlinger 2>/dev/null | grep -iE "Display .* HWC|activeConfig|refresh|1080|1920|3664|density|configs" | head -20
echo "--- drm/fb ---"
ls /sys/class/drm/ 2>/dev/null | tr '\n' ' '; echo
cat /sys/class/drm/*/modes 2>/dev/null | head -5

echo
echo "=========== 5. lens / display calibration on persist ==========="
ls -lR /persist/pvr 2>/dev/null | head -40
ls -l /persist/ 2>/dev/null | head -20

echo
echo "=========== 6. QVR service client API surface ==========="
echo "--- libqvrservice_client exported symbols ---"
strings -a /vendor/lib64/libqvrservice_client.so 2>/dev/null | grep -E "^QVRService" | sort -u | head -60

echo
echo "=========== 7. QVR camera client API surface ==========="
strings -a /vendor/lib64/libqvrcamera_client.so 2>/dev/null | grep -E "^QVRCamera" | sort -u | head -40

echo
echo "=========== 8. GPU driver: kgsl present for Turnip? ==========="
ls -l /dev/kgsl-3d0 2>/dev/null
cat /sys/class/kgsl/kgsl-3d0/gpu_model /sys/class/kgsl/kgsl-3d0/max_gpuclk 2>/dev/null
echo "--- vulkan blob ---"
ls -l /vendor/lib64/hw/vulkan.sdm845.so 2>/dev/null

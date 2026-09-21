#!/system/bin/sh
# What API surface would a custom runtime (Monado/OpenXR) target? QVR is
# Qualcomm's, not Pico's - if it is rich enough we can bypass Pico's entire
# 8.1-era stack, which is where every problem in this port has come from.
echo "=== QVR client entry points ==="
strings -a /system/lib64/libqvrservice_client.so 2>/dev/null | grep -E '^QVRServiceClient_' | sort -u
echo
echo "=== QVR camera client entry points ==="
strings -a /system/lib64/libqvrcamera_client.so 2>/dev/null | grep -E '^QVRCameraClient_|^QVRCameraDevice_' | sort -u | head -25
echo
echo "=== lens / display parameters available to us ==="
grep -iE 'lens|fov|ipd|distortion' /system/etc/pvr/psmvrapi_config.txt 2>/dev/null | head -20

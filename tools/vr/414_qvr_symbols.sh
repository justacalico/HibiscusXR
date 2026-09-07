#!/system/bin/sh
# Does qvrservice itself do the 6DoF, or is it just a camera/vsync pump?
# qvrservicetest64 is Qualcomm's own client sample and it is sitting on the device.
echo "=========== what qvrservicetest64 imports from the client lib ==========="
strings -a /vendor/bin/qvrservicetest64 | grep -E "QVRServiceClient_|_ZN16QVR|GetHeadTracking|GetPose|TrackingMode|SetVRMode" | sort -u | head -60

echo
echo "=========== its usage text (tells us the feature surface) ==========="
strings -a /vendor/bin/qvrservicetest64 | grep -iE "usage|-[a-z] +<|tracking|pose|dof|mode" | head -40

echo
echo "=========== client lib: exported C++ mangled names ==========="
strings -a /system/lib64/libqvrservice_client.so | grep -E "^_Z.*QVR" | sort -u | head -80

echo
echo "=========== camera client: exported C++ mangled names ==========="
strings -a /system/lib64/libqvrcamera_client.so | grep -E "^_Z.*QVR" | sort -u | head -60

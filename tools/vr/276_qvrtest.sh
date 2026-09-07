#!/system/bin/sh
# /system/bin/qvrservicetest is Qualcomm's own client. If it can create a client
# and read tracking modes, the service side is fine and the problem is in Pico's
# client path. If it fails the same way, the fault is in qvrservice itself.
echo "=== rest of qvrservice startup ==="
logcat -d 2>/dev/null | grep -iE 'QVRService|qvrservice_main|QVRServiceCam|QVRServiceConnection' | tail -25
echo
echo "=== socket present? ==="
ls -lZ /dev/socket/qvrservice* 2>/dev/null
echo
echo "=== qvrservicetest (Qualcomm's own client) ==="
/system/bin/qvrservicetest 2>&1 | head -30
echo "--- exit: $? ---"

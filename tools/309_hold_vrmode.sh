#!/system/bin/sh
# airservice logs "vr mode is not started 0" and gives up after 10 retries; the
# cameras only open once QVR VR mode is running. qvrservicetest starts VR mode for
# the duration of its run, so hold it open and watch whether the camera pipeline
# comes up behind it.
logcat -c
echo "=== starting VR mode via qvrservicetest (30s) ==="
/vendor/bin/qvrservicetest64 >/dev/null 2>&1 &
TESTPID=$!
sleep 6

echo "--- is VR mode up? ---"
logcat -d 2>/dev/null | grep -iE 'VR Mode started|starting VR mode' | tail -3

echo
echo "--- kick airservice so it retries the camera now ---"
stop airservice; sleep 1; start airservice
sleep 10

echo
echo "=== camera pipeline ==="
logcat -d 2>/dev/null | grep -iE 'QVRServiceCamDeviceHAL3|openCamera|QVRServiceClient_Create|CAMERA_READY|camera' | tail -20

echo
echo "=== airservice state ==="
logcat -d 2>/dev/null | grep -iE 'AIRService' | tail -8

echo
echo "=== is airservice published now? ==="
service list 2>/dev/null | grep -i air

kill $TESTPID 2>/dev/null

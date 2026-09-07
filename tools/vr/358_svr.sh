#!/system/bin/sh
# poseStatus is 0 while the pose values are valid. In the Snapdragon VR (svr)
# layer, poseStatus is a bitmask (position/rotation valid) set from QVR's tracking
# state. Read what the svr layer actually reports.
logcat -c
am force-stop com.pvr.vrshell
sleep 2
stop pvrservice; sleep 2; start pvrservice; sleep 6
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 18
echo "=== everything the svr layer logs ==="
logcat -d | grep -E " svr *:" | head -40
echo
echo "=== did anything START vr mode on QVR? ==="
logcat -d | grep -iE "StartVRMode|start VR mode|VR Mode started|SetTrackingMode|GetTrackingMode|tracking mode" | tail -12
echo
echo "=== QVR service side ==="
logcat -d | grep -iE "^.*QVRService[^C]" | grep -viE "LockBuffer|CamDevice" | tail -15

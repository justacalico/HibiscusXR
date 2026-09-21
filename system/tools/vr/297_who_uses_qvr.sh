#!/system/bin/sh
# QVR works when Qualcomm's own test drives it, but the app still reads
# trackingstate 0. Who actually connects to qvrservice, and does anyone start VR
# mode?
echo "=== QVR connections since boot ==="
logcat -d 2>/dev/null | grep -iE 'QVRConnection|QVRServiceConnectionMgr|QVRClientImpl|VR Mode' | tail -20
echo
echo "=== does pvrservice talk to QVR at all? ==="
P=$(pidof pvrservice)
echo "pvrservice pid $P"
grep -oE 'libqvr[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo "--- its fds on the qvr socket ---"
ls -l /proc/$P/fd 2>/dev/null | grep -c socket
echo
echo "=== does VRShell load the qvr client? ==="
V=$(pidof com.pvr.vrshell)
echo "vrshell pid $V"
grep -oE 'libqvr[^ ]*\.so' /proc/$V/maps 2>/dev/null | sort -u
echo
echo "=== what the SDK logs around tracking ==="
logcat -d 2>/dev/null | grep -iE 'QVRServiceClient|supportedTracking|GetTrackingMode|SetTrackingMode|trackingstate' | tail -15

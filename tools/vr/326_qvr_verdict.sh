#!/system/bin/sh
# libpvrmodule_platform asks QVR whether VR is supported and logs the verdict.
# What did it get this boot?
echo "=== pvrservice's QVR verdict ==="
logcat -d 2>/dev/null | grep -iE 'QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create|svr ' | tail -15
echo "  (blank = it has not asked yet this boot)"
echo
echo "=== is QVR itself healthy right now? ==="
logcat -c
timeout 10 /vendor/bin/qvrservicetest64 2>&1 | grep -iE 'supported|starting VR mode|Api version'
sleep 1
logcat -d 2>/dev/null | grep -iE 'DspWrapper|MapperWrapper|VR mode|Plugin not valid' | tail -6
echo
echo "=== now force pvrservice to re-ask by restarting it ==="
stop pvrservice; sleep 2; start pvrservice; sleep 8
logcat -d 2>/dev/null | grep -iE 'QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create' | tail -6
echo
echo "=== did it load the qvr client this time? ==="
P=$(pidof pvrservice)
grep -oE 'libqvr[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo "  (blank = still not loaded)"

#!/system/bin/sh
# The SDK's tracking state comes from svrGetSupportedTrackingModes (SnapdragonVR),
# not directly from QVR. There is even an error string for it:
#   "svrGetSupportedTrackingModes failed: SnapdragonVR not initialized!"
# Is SVR initialising in the VR app?
echo "=== svr / SnapdragonVR log lines ==="
logcat -d 2>/dev/null | grep -iE '\bsvr\b|SnapdragonVR|svrapi|svrInit|svrBeginVr' | tail -25
echo
echo "=== does the app load libsvrapi? ==="
V=$(pidof com.pvr.vrshell)
echo "vrshell pid $V"
grep -oE '/[^ ]*svr[^ ]*\.so' /proc/$V/maps 2>/dev/null | sort -u
echo
echo "=== where does libsvrapi.so live ==="
ls -l /system/etc/pvr/libsvrapi.so /system/lib64/libsvrapi.so /system/lib/libsvrapi.so 2>/dev/null
echo
echo "=== is it in the linker whitelist? ==="
grep -i svr /system/etc/public.libraries.txt 2>/dev/null || echo "  NOT whitelisted"

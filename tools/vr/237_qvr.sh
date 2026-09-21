#!/system/bin/sh
# airservice gets NULL from QVRServiceClient_Create. QVR is Qualcomm's VR service
# and owns the camera path. Is it running, and where does it come from?
echo "=== qvr processes ==="
ps -A 2>/dev/null | grep -iE 'qvr|vr_service|xr'
echo
echo "=== registered services mentioning vr/qvr ==="
service list 2>/dev/null | grep -iE 'qvr|vr'
echo
echo "=== qvr binaries (vendor is untouched, so these should exist) ==="
ls -l /vendor/bin/ 2>/dev/null | grep -iE 'qvr'
ls -l /system/bin/ 2>/dev/null | grep -iE 'qvr'
echo
echo "=== qvr init entries ==="
grep -rl -i qvr /vendor/etc/init/ /system/etc/init/ 2>/dev/null
echo "--- service blocks ---"
grep -rh -A5 -iE '^service +[a-z_]*qvr' /vendor/etc/init/ /system/etc/init/ 2>/dev/null | head -30
echo
echo "=== init state of anything qvr ==="
getprop 2>/dev/null | grep -iE 'init\.svc.*qvr|qvr'
echo
echo "=== qvr libs ==="
ls /vendor/lib64 /vendor/lib /system/lib64 2>/dev/null | grep -i qvr | sort -u

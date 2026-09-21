#!/system/bin/sh
# QVR now reports "VR mode is supported", but pvrservice never asks it.
# On stock, pvrservice DOES load libqvrservice_client. Compare their startup.
echo "=== restart pvrservice and capture everything it logs ==="
logcat -c
stop pvrservice
sleep 2
start pvrservice
sleep 10
P=$(pidof pvrservice)
echo "pid $P"
echo
echo "--- its startup lines ---"
logcat -d 2>/dev/null | grep " $P " | grep -viE 'getTrackingDataExt|chatty|stationary' | head -40
echo
echo "--- 6dof decisions ---"
logcat -d 2>/dev/null | grep -iE '3dof|6dof|sensor.source|tracker|svr' | head -15

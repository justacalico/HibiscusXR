#!/system/bin/sh
# How much of Pico's VR stack survived swapping /system for the GSI?
# Anything under /vendor is still here. Anything that lived on the stock /system
# is gone and would have to be re-installed for a compositor port.
echo "=== VR services running right now ==="
for s in qvrservice qvrd pvrservice vr_hwc sensors.qti; do
  echo "$s = $(getprop init.svc.$s)"
done
ps -A 2>/dev/null | grep -iE 'qvr|pvr|vr_hwc' || echo "(no qvr/pvr processes)"
echo

echo "=== VR composer HAL (the direct-mode path) ==="
lshal 2>/dev/null | grep -iE 'composer.*vr|IComposer/vr'
echo

echo "=== what VR bits exist on /vendor (survived) ==="
ls -l /vendor/bin/ 2>/dev/null | grep -iE 'qvr|pvr|vr'
echo "--- libs ---"
ls /vendor/lib64/ 2>/dev/null | grep -iE 'qvr|pvr|compositor|openxr|svr'
ls /vendor/lib/ 2>/dev/null | grep -iE 'qvr|pvr|compositor|openxr|svr'
echo "--- vendor etc configs ---"
ls /vendor/etc/ 2>/dev/null | grep -iE 'qvr|pvr|vr'
echo

echo "=== VR bits on the current /system (GSI - expect almost nothing) ==="
ls /system/lib64/ 2>/dev/null | grep -iE 'qvr|pvr|compositor'
ls /system/priv-app/ 2>/dev/null | grep -iE 'pvr|pico|vr'
ls /system/app/ 2>/dev/null | grep -iE 'pvr|pico|vr'
echo

echo "=== does the framework expose VR mode at all? ==="
pm list features 2>/dev/null | grep -i vr
dumpsys vrmanager 2>/dev/null | head -10
echo

echo "=== persist calibration still intact? ==="
ls -l /persist/pvr/camera/ 2>/dev/null | head
echo

echo "=== sensors available to an NDK app (what my renderer will use) ==="
dumpsys sensorservice 2>/dev/null | grep -iE '^[0-9a-fx]+\)|Rotation Vector|Gyroscope|Accelerometer' | head -20
echo DONE

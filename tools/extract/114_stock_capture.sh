#!/system/bin/sh
# RUN THIS ON THE STOCK NEO 2 (the Eye unit with PUI), not the port.
#
# Read-only. Captures what a WORKING VRShell actually loads, so we can diff it
# against ours instead of discovering each missing library by crashing into it.
#
#   adb push 114_stock_capture.sh /data/local/tmp/
#   adb shell sh /data/local/tmp/114_stock_capture.sh > stock_capture.txt
#
# If VRShell is already running this needs no root. Root only helps for dmesg.
OUT() { echo; echo "##### $* #####"; }

OUT identity
getprop ro.product.device
getprop ro.build.version.release
getprop ro.pvr.internal.version
getprop ro.build.display.id

OUT vrshell pid
PID=$(pidof com.pvr.vrshell)
echo "pid=$PID"
if [ -z "$PID" ]; then
  echo "VRShell is not running - open the Pico home/shell first, then re-run"
fi

OUT every shared library VRShell has mapped
# this is the artifact that matters: the complete, correct dependency set
[ -n "$PID" ] && grep -oE '/[^ ]*\.so' /proc/$PID/maps | sort -u

OUT which of those are 6dof/tracking related
[ -n "$PID" ] && grep -oE '/[^ ]*\.so' /proc/$PID/maps | sort -u | grep -iE '6dof|reset|track|pxr|pvr|svr'

OUT pvr/pxr properties
getprop | grep -iE 'pvr|pxr|psmart|6dof|tracking'

OUT pvr services running
getprop | grep -E 'init\.svc\.(pvr|qvr|pxr)'
ps -A 2>/dev/null | grep -iE 'pvr|qvr|pxr'

OUT system libs present
ls /system/lib64/ | grep -iE '6dof|pxr|pvr|qvr|svr|reset'
echo "--- lib ---"
ls /system/lib/ | grep -iE '6dof|pxr|pvr|qvr|svr|reset'

OUT vrshell app-private libs
ls -l /system/priv-app/VRShell2/lib/arm64/ 2>/dev/null

OUT etc/pvr
ls /system/etc/pvr/ 2>/dev/null

echo
echo "##### DONE #####"

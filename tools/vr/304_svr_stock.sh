#!/system/bin/sh
# Does stock's VRShell load libsvrapi, and from where?
V=$(pidof com.pvr.vrshell)
echo "vrshell pid ${V:-<not running>}"
if [ -n "$V" ]; then
  echo "--- svr libs mapped ---"
  grep -oE '/[^ ]*svr[^ ]*\.so' /proc/$V/maps 2>/dev/null | sort -u
  echo "--- all pvr/etc libs mapped ---"
  grep -oE '/system/etc/pvr/[^ ]*\.so' /proc/$V/maps 2>/dev/null | sort -u
fi
echo
echo "=== where libsvrapi.so exists here ==="
ls -l /system/etc/pvr/libsvrapi.so /system/lib64/libsvrapi.so /system/lib/libsvrapi.so 2>/dev/null
echo
echo "=== svr lines in the log ==="
logcat -d 2>/dev/null | grep -iE '\bsvr\b|SnapdragonVR|svrapi' | tail -15

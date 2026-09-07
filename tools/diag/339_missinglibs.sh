#!/system/bin/sh
echo "=== the three modules pvrservice could not open ==="
for l in libHeadImuCalibrate_int.so lib6DofReset.so libpvrmodule_externalhmd.so; do
  printf '%-32s' "$l"
  F=$(find /system /vendor -name "$l" 2>/dev/null | head -3 | tr '\n' ' ')
  [ -n "$F" ] && echo "$F" || echo "ABSENT"
done
echo
echo "=== who holds the pvrservice binder (if registered) ==="
service check pvrservice 2>/dev/null
service check airservice 2>/dev/null
echo
echo "=== does pvrservice reference addService at all? ==="
strings -a /system/bin/pvrservice 2>/dev/null | grep -iE "addService|PvrService|android.Service" | head -10
echo
echo "=== does the PLATFORM module do the registering? ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -iE "addService|android.Service.PvrService" | head -5
strings -a /system/lib64/libpvrservice.so 2>/dev/null | grep -iE "addService|android.Service.PvrService" | head -5
echo
echo "=== any lib that names the binder descriptor ==="
grep -rl "android.Service.PvrService" /system/lib64 /system/lib /system/bin 2>/dev/null | head -5

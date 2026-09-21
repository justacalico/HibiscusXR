#!/system/bin/sh
# The _system copy is rejected on symbol versioning: the stub's verneed refers to
# SONAME libcdsprpc.so and ours says libcdsprpc_system.so. The /vendor copy has
# the right SONAME. Check what it drags in before putting it in /system.
echo "=== /vendor/lib/libcdsprpc.so ==="
readelf -dW /vendor/lib/libcdsprpc.so 2>/dev/null | grep -E 'SONAME|NEEDED'
echo
echo "=== are those deps reachable from /system? ==="
for n in $(readelf -dW /vendor/lib/libcdsprpc.so 2>/dev/null | grep NEEDED | sed 's/.*\[\(.*\)\]/\1/'); do
  if [ -f "/system/lib/$n" ]; then echo "  system   $n"
  elif [ -f "/apex/com.android.runtime/lib/bionic/$n" ]; then echo "  bionic   $n"
  elif [ -f "/vendor/lib/$n" ]; then echo "  VENDOR   $n   <-- not visible to a /system process"
  else echo "  MISSING  $n"; fi
done
echo
echo "=== version definitions each copy provides ==="
echo "--- vendor libcdsprpc.so ---"
readelf -VW /vendor/lib/libcdsprpc.so 2>/dev/null | grep -A3 'Version definition' | head -12
echo "--- our system copy ---"
readelf -VW /system/lib/libcdsprpc.so 2>/dev/null | grep -A3 'Version definition' | head -12
echo
echo "=== what the stub requires ==="
readelf -VW /system/lib/libqvr_cdsp_driver_stub.so 2>/dev/null | grep -A6 'Version needs' | head -20

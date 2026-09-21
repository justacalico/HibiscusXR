#!/system/bin/sh
# libqvr_mapper_stub.so needs libmdsprpc.so (modem DSP RPC). Same shape as the
# libcdsprpc.so problem. Find every copy and its SONAME/version needs.
echo "=== libmdsprpc anywhere ==="
for d in /system/lib /system/lib64 /vendor/lib /vendor/lib64 /system/vendor/lib /system/vendor/lib64; do
  [ -d "$d" ] || continue
  ls -l "$d" 2>/dev/null | grep -i 'mdsprpc' | sed "s|^|  $d/ |"
done
echo
echo "=== all *dsprpc* copies for comparison ==="
ls /vendor/lib/*dsprpc*.so /system/lib/*dsprpc*.so 2>/dev/null

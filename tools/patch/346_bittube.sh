#!/system/bin/sh
S=_ZN7android7BitTubeC1ERKNS_6ParcelE
echo "=== who exports the old unnamespaced android::BitTube? (32-bit) ==="
for d in /system/lib /vendor/lib /system/lib/vndk-27 /system/lib/vndk-sp-27 /apex/com.android.vndk.v27/lib; do
  [ -d "$d" ] || continue
  for f in "$d"/*.so; do
    grep -q "$S" "$f" 2>/dev/null && echo "  $f"
  done
done
echo
echo "=== same, 64-bit ==="
for d in /system/lib64 /vendor/lib64 /system/lib64/vndk-27 /apex/com.android.vndk.v27/lib64; do
  [ -d "$d" ] || continue
  for f in "$d"/*.so; do
    grep -q "$S" "$f" 2>/dev/null && echo "  $f"
  done
done
echo
echo "=== do we have a vndk-27 tree at all? ==="
ls -d /system/lib/vndk* /system/lib64/vndk* /apex/com.android.vndk* 2>/dev/null || echo "  none"

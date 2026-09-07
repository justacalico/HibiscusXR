#!/system/bin/sh
# pvrservice picks its platform profile from device identity and falls back to
# _G2U3_PU (Pico G2 4K) when it cannot match. On the port it always picks G2U3
# even after setting ro.product.model, and the remaining difference is:
#
#   ro.build.product   STOCK=PICOA7B10   PORT=phhgsi_arm64_a
#
# ro.* cannot be re-set at runtime once init has set it, so the existing line in
# build.prop has to be edited rather than appended - for ro.* the first
# definition wins, so appending would do nothing.
exec 2>&1
mount -o rw,remount /system

echo "=== current identity lines ==="
grep -nE '^ro\.(build\.product|product\.(device|name|model|brand|manufacturer))=' /system/build.prop

cp -f /system/build.prop /system/build.prop.bak 2>/dev/null

# rewrite in place; add the ones that are missing entirely
sed -i 's/^ro\.build\.product=.*/ro.build.product=PICOA7B10/' /system/build.prop
grep -q '^ro.build.product=' /system/build.prop || echo 'ro.build.product=PICOA7B10' >> /system/build.prop

for kv in "ro.product.device=PICOA7B10" "ro.product.name=PICOA7B10" \
          "ro.product.model=Pico Neo 2" "ro.product.brand=Pico" \
          "ro.product.manufacturer=Pico"; do
  k=$(echo "$kv" | cut -d= -f1)
  if grep -q "^$k=" /system/build.prop; then
    v=$(echo "$kv" | cut -d= -f2-)
    sed -i "s|^$k=.*|$k=$v|" /system/build.prop
  else
    echo "$kv" >> /system/build.prop
  fi
done

echo
echo "=== after ==="
grep -nE '^ro\.(build\.product|product\.(device|name|model|brand|manufacturer))=' /system/build.prop
sync
mount -o ro,remount /system
echo DONE

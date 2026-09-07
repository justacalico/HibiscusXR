#!/system/bin/sh
echo "=========== identity: what does this specific unit call itself ==========="
for p in ro.serialno ro.boot.serialno ro.product.model ro.product.name \
         persist.pvr.sn persist.pvr.pn persist.pvr.hwversion ro.pvr.model \
         persist.pvr.device.model persist.sys.pvr.sku; do
  v=$(getprop $p); [ -n "$v" ] && echo "  $p = $v"
done
getprop | grep -iE "pico|pvr" | grep -iE "sn|serial|model|sku|hw|product|version" | head -20

echo
echo "=========== is /persist its own partition (survives a system flash)? ==========="
mount | grep -iE " /persist | /vendor | /system "
echo "--- persist usage ---"
df -h /persist 2>/dev/null | tail -2

echo
echo "=========== which calibration file belongs to THIS unit ==========="
echo "adb serial is PA7B40NGE5300009W"
ls -l /persist/calibration/*.txt 2>/dev/null
echo "--- head of each, looking for a serial/UID field ---"
for f in /persist/calibration/PC1610*.txt; do
  echo "  == $f =="; head -c 220 "$f"; echo
done

echo
echo "=========== display / panel calibration ==========="
ls -l /persist/display /persist/calibration/Bosh /persist/calibration/temp 2>/dev/null
head -c 200 /persist/calibration/Gyrooffset.txt 2>/dev/null; echo

echo
echo "=========== SKU detection: does this unit have eye tracking hw? ==========="
grep -E "^eye-tracking_camera_id" /vendor/etc/qvr/qvrservice_config*.txt 2>/dev/null
ls -l /persist/pvr/tobstatus 2>/dev/null; cat /persist/pvr/tobstatus 2>/dev/null; echo

echo
echo "=========== is /vendor untouched stock (can we rely on it being there)? ==========="
getprop ro.vendor.build.fingerprint
getprop ro.vendor.build.date
echo "  vendor mounted: $(mount | grep -c ' /vendor ')"

#!/system/bin/sh
# Why does IBootControl::getService() block instead of returning null?
# hidl only waits forever for interfaces DECLARED in a VINTF manifest.
echo "=== does any manifest declare android.hardware.boot? ==="
grep -rl 'hardware.boot' /vendor/etc/vintf/ /system/etc/vintf/ /odm/etc/vintf/ 2>/dev/null
echo "---"
for f in /vendor/etc/vintf/manifest.xml /vendor/manifest.xml /system/etc/vintf/manifest.xml; do
  echo "## $f"
  grep -A6 'hardware\.boot' "$f" 2>/dev/null
done
echo

echo "=== boot HAL binary present in vendor? ==="
ls -l /vendor/bin/hw/ 2>/dev/null | grep -i boot
ls -l /vendor/lib64/hw/ 2>/dev/null | grep -i bootctrl
echo

echo "=== is it registered / running? ==="
lshal 2>/dev/null | grep -i 'hardware.boot'
getprop | grep -i 'boot@'
echo

echo "=== A/B evidence ==="
getprop ro.boot.slot_suffix
getprop ro.build.ab_update
ls /dev/block/bootdevice/by-name/ | grep -iE '_a$|_b$' | head
echo

echo "=== checkpoint metadata file vold also consults ==="
ls -l /metadata 2>/dev/null
cat /metadata/vold/checkpoint 2>/dev/null
echo

echo "=== hwservicemanager: anything waiting? ==="
lshal --types=all 2>/dev/null | grep -iE 'boot|N/A' | head -20
echo DONE

#!/system/bin/sh
# The see-through app blocks on "Waiting for service 'airservice'". Find what
# provides it on stock and whether we ship that binary/init entry at all.
echo "=== is airservice registered? ==="
service list 2>/dev/null | grep -i air
echo "--- running processes ---"
ps -A 2>/dev/null | grep -iE 'air|pvr|cv'
echo
echo "=== binaries / init entries mentioning airservice ==="
ls -l /system/bin/ 2>/dev/null | grep -iE 'air'
grep -rl airservice /system/etc/init/ 2>/dev/null
echo "--- init rc contents ---"
grep -rh -A6 -iE 'service +air' /system/etc/init/ 2>/dev/null | head -20
echo
echo "=== libs mentioning airservice ==="
ls /system/lib64 /system/lib 2>/dev/null | grep -iE 'air' | sort -u

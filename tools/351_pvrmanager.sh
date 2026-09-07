#!/system/bin/sh
# VRShell logs "PvrManagerService dont instantiate properly" and "VirtualDisplay
# is null", then never calls EnterVrMode. pvr_manager is a Pico addition to
# their system_server; ours is LineageOS's. Find out how it ships.
echo "=== pico jars in /system/framework ==="
ls /system/framework/ 2>/dev/null | grep -iE "pvr|pico|pxr"
echo
echo "=== does any framework jar mention PvrManagerService ==="
for f in /system/framework/*.jar /system/framework/*.apk; do
  grep -q "PvrManagerService" "$f" 2>/dev/null && echo "  $f"
done
echo
echo "=== services.jar: does it contain the Pico service? ==="
grep -c "PvrManagerService" /system/framework/services.jar 2>/dev/null
echo
echo "=== framework permissions / features that declare it ==="
grep -rl "pvr" /system/etc/permissions/ 2>/dev/null | head
echo
echo "=== which lib provides PvrManagerInternal ==="
for f in /system/framework/*.jar; do
  grep -q "PvrManagerInternal" "$f" 2>/dev/null && echo "  $f"
done
echo
echo "=== is there a pvr framework jar in the classpath ==="
getprop | grep -iE "bootclasspath|systemserverclasspath" | head -3

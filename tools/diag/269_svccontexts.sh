#!/system/bin/sh
# Pico's daemons run but never appear in `service list`. Android's servicemanager
# refuses addService() for a name it cannot resolve in service_contexts, and ours
# is the GSI's file - it will not contain Pico's service names.
echo "=== pico service names in service_contexts ==="
for f in /system/etc/selinux/plat_service_contexts /vendor/etc/selinux/vndservice_contexts /system/etc/selinux/plat_hwservice_contexts; do
  [ -f "$f" ] || continue
  echo "--- $f ---"
  grep -iE 'pvr|pico|air|psmart|pxr' "$f" 2>/dev/null || echo "  (no pico entries)"
done
echo
echo "=== servicemanager denials in the log ==="
logcat -d 2>/dev/null | grep -iE 'servicemanager|add_service|avc.*service_manager' | tail -20
echo
echo "=== what IS registered ==="
service list 2>/dev/null | head -20

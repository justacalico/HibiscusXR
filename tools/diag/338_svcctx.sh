#!/system/bin/sh
# Q's servicemanager refuses addService for any name with no entry in
# service_contexts. Pico's 8.1 policy had those entries; the GSI's does not.
echo "=== do we have entries for the pvr services? ==="
for n in pvrservice airservice pvr_manager ConfigurationService CVControllerService; do
  printf '%-22s ' "$n"
  grep -h "^$n " /system/etc/selinux/plat_service_contexts /vendor/etc/selinux/vendor_service_contexts 2>/dev/null | head -1 || true
  grep -qh "^$n " /system/etc/selinux/plat_service_contexts /vendor/etc/selinux/vendor_service_contexts 2>/dev/null || echo "MISSING"
done
echo
echo "=== servicemanager complaints ==="
logcat -d 2>/dev/null | grep -iE "servicemanager|service_contexts|add_service|avc.*service_manager" | tail -15
echo
echo "=== what pvrservice itself says about registering ==="
logcat -d 2>/dev/null | grep -iE "addService|publish|IServiceManager|defaultServiceManager" | tail -10
echo
echo "=== tail of plat_service_contexts (format reference) ==="
tail -4 /system/etc/selinux/plat_service_contexts
echo
echo "=== is there a spare type we can reuse? ==="
grep -cE '^[a-zA-Z]' /system/etc/selinux/plat_service_contexts
grep -E "default_android_service" /system/etc/selinux/plat_service_contexts | head -3

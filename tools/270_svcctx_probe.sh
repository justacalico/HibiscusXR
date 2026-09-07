#!/system/bin/sh
# We cannot reference stock's airservice_service type - our GSI policy does not
# define it, and servicemanager rejects a context that fails validation. Find a
# type our policy DOES have that we can map Pico's service names onto.
F=/system/etc/selinux/plat_service_contexts
echo "=== tail of our service_contexts (format + default entry) ==="
tail -8 "$F"
echo
echo "=== is there a default_android_service entry? ==="
grep -nE 'default_android_service|^\*' "$F" | head
echo
echo "=== does the policy define these types? ==="
for t in default_android_service airservice_service surfaceflinger_service; do
  if [ -e /sys/fs/selinux/context ]; then
    printf "  %-28s " "$t"
    echo "u:object_r:$t:s0" > /sys/fs/selinux/context 2>/dev/null && echo "VALID" || echo "invalid/unknown"
  fi
done
echo
echo "=== which pico services fail to register right now ==="
for s in pvrservice pvr_manager ConfigurationService airservice; do
  printf "  %-22s %s\n" "$s" "$(service check $s 2>/dev/null)"
done

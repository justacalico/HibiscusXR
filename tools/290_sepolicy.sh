#!/system/bin/sh
# The vendor sepolicy DEFINES qvrd but the loaded policy does not know it. On
# Treble, precompiled_sepolicy is only used when its plat hash matches the running
# system; otherwise vendor domains are dropped. That would explain the unlabeled
# contexts and u:r:shell:s0 denials seen all session.
echo "=== is SELinux enforcing? ==="
getenforce
echo
echo "=== policy version / files ==="
cat /sys/fs/selinux/policyvers 2>/dev/null | sed 's/^/  policyvers: /'
ls -l /vendor/etc/selinux/ 2>/dev/null
echo
echo "=== do the plat hashes match? (this decides if vendor policy loads) ==="
for f in /vendor/etc/selinux/precompiled_sepolicy.plat_sepolicy_and_mapping.sha256 \
         /vendor/etc/selinux/precompiled_sepolicy.plat_and_mapping.sha256; do
  [ -f "$f" ] && echo "  vendor expects: $(cat $f)"
done
for f in /system/etc/selinux/plat_sepolicy_and_mapping.sha256 /system/etc/selinux/plat_and_mapping.sha256; do
  [ -f "$f" ] && echo "  system provides: $(cat $f)"
done
echo
echo "=== a few vendor domains - known to the running policy? ==="
for d in qvrd hal_sensors_default vendor_init; do
  printf "  %-22s " "$d"
  echo "u:r:$d:s0" > /sys/fs/selinux/context 2>/dev/null && echo "valid" || echo "NOT in loaded policy"
done
echo
echo "=== how many contexts does the running policy know ==="
cat /sys/fs/selinux/policy 2>/dev/null | wc -c | sed 's/^/  policy size: /'

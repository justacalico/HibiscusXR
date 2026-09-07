#!/system/bin/sh
# Only difference between our qvrservice and stock's is the SELinux domain:
#   stock  u:r:qvrd:s0
#   ours   u:r:shell:s0   (I picked shell just to satisfy init's requirement)
# The vendor partition is Pico's, so vendor sepolicy probably defines qvrd. If the
# domain exists, use it - FastRPC/DSP authorisation can depend on the caller's
# context even when SELinux is permissive.
echo "=== is u:r:qvrd:s0 a valid context here? ==="
if [ -w /sys/fs/selinux/context ]; then
  echo "u:r:qvrd:s0" > /sys/fs/selinux/context 2>/dev/null && echo "  VALID" || echo "  invalid / not defined"
else
  echo "  cannot test via /sys/fs/selinux/context"
fi
echo
echo "=== does the loaded policy know the type? ==="
if [ -e /sys/fs/selinux/class/file/perms ]; then
  seinfo -t 2>/dev/null | grep -w qvrd || echo "  (seinfo unavailable)"
fi
grep -rw qvrd /vendor/etc/selinux/*contexts* 2>/dev/null | head
echo
echo "=== what contexts exist for qvr in vendor policy files ==="
strings /vendor/etc/selinux/precompiled_sepolicy 2>/dev/null | grep -iE '^qvr' | sort -u | head
strings /vendor/etc/selinux/vendor_sepolicy.cil 2>/dev/null | grep -iE 'qvrd' | head -5

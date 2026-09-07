#!/system/bin/sh
# On stock, pvrservice AND CVService link libqvrservice_client.so. On ours nothing
# does. Find out whether pvrservice tries and fails.
echo "=== dlopen failures in pvrservice ==="
logcat -d 2>/dev/null | grep -iE 'pvrservice|PvrService' | grep -iE 'dlopen|cannot|failed|qvr|library' | tail -15
echo
echo "=== is libqvrservice_client in the linker whitelist? ==="
grep -i qvr /system/etc/public.libraries.txt 2>/dev/null || echo "  NOT whitelisted"
echo
echo "=== does pvrservice reference it at all? ==="
strings -a /system/bin/pvrservice 2>/dev/null | grep -iE 'qvr' | sort -u | head
echo "--- and its libs ---"
for l in /system/lib64/libpvrservice.so /system/lib64/libpvrmodule_platform.so /system/lib64/libpvrmodule_orientationtracker.so; do
  [ -f "$l" ] || continue
  s=$(strings -a "$l" 2>/dev/null | grep -icE 'qvr')
  echo "  $(basename $l): $s qvr references"
  strings -a "$l" 2>/dev/null | grep -iE 'libqvr[a-z_]*\.so' | sort -u | sed 's/^/     /'
done
echo
echo "=== CVService state (it holds VR mode on stock) ==="
pm list packages -d 2>/dev/null | grep -i cvcontroller
dumpsys package com.picovr.picovrlib.cvcontroller 2>/dev/null | grep -iE 'enabled=' | head -2

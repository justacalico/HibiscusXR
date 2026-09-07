#!/system/bin/sh
P=$(pidof pvrservice)
echo "=== pvr/qvr modules in pvrservice (pid $P) ==="
grep -oE '/system/lib64/lib(pvr|qvr|pxr)[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo
echo "=== dlopen failures ==="
logcat -d 2>/dev/null | grep -iE 'dlopen failed|cannot locate|CANNOT LINK' | tail -8
echo "  (blank = none)"
echo
echo "=== qvr strings in libpvrmodule_platform ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -iE 'qvr' | sort -u | head -15
echo
echo "=== what gates it - look for a property or config name ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -E '^(persist|ro|pvr|sys)\.[a-z0-9_.]+$' | sort -u | head -20

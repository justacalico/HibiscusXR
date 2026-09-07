#!/system/bin/sh
echo "=== boot state ==="
echo "boot_completed : $(getprop sys.boot_completed)"
echo "bootanim       : $(getprop init.svc.bootanim)"
echo "zygote         : $(getprop init.svc.zygote)"
echo "surfaceflinger : $(getprop init.svc.surfaceflinger)"
echo "vendor.qvr     : $(getprop init.svc.vendor.qvrservice)"
echo "uptime         : $(cat /proc/uptime)"
echo

echo "=== mounts ==="
cat /proc/mounts | grep -E "system|vendor|oem|dsp|persist"
echo

echo "=== is /vendor populated? ==="
ls /vendor/ 2>&1 | head -20
echo "vendor/lib64 count: $(ls /vendor/lib64 2>/dev/null | wc -l)"
echo "vendor/bin/hw count: $(ls /vendor/bin/hw 2>/dev/null | wc -l)"
echo

echo "=== vendor build.prop ==="
head -5 /vendor/build.prop 2>&1
echo

echo "=== running processes (top 25 by name) ==="
ps -A -o NAME 2>/dev/null | sort -u | head -40
echo

echo "=== recent kernel messages ==="
dmesg 2>/dev/null | tail -25
echo

echo "=== logcat: errors and fatals ==="
logcat -d -b main,system,crash *:E 2>/dev/null | tail -40

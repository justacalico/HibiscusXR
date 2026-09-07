#!/system/bin/sh
# Android 10 sensorservice LOG(FATAL)s on any sensor event whose type is not a
# known standard type and is below DEVICE_PRIVATE_BASE (65536). Pico emits type
# 126, so system_server dies and the device reboots. Find what 126 is.
echo "=== full sensor list with types ==="
dumpsys sensorservice 2>/dev/null | sed -n '1,/^$/p' | head -60
echo
echo "=== anything reporting type 126 / 0x7e ==="
dumpsys sensorservice 2>/dev/null | grep -iE '\(126\)|type=126|126\)'
echo
echo "=== pico sensor HAL config ==="
ls -l /vendor/etc/sensors/ 2>/dev/null | head -20
ls -l /vendor/etc/pvr/ 2>/dev/null | head -20
echo
echo "=== pvr sensor drivers loaded by the HAL ==="
ls -l /vendor/lib64/sensors*.pvr.so /vendor/lib/sensors*.pvr.so 2>/dev/null
echo
echo "=== which process serves sensors@1.0 ==="
lshal 2>/dev/null | grep -i 'sensors@'
echo
echo "=== how many times has this killed us? ==="
logcat -d -b crash 2>/dev/null | grep -c 'DEVICE_PRIVATE_BASE'
echo
echo "=== uptime / boot count evidence ==="
echo "uptime $(cut -d. -f1 /proc/uptime)s"
echo DONE

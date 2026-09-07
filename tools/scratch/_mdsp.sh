mount -o rw,remount /system
# same fix as libcdsprpc: the /vendor copy has the SONAME the stub verneeds
cp -f /vendor/lib/libmdsprpc.so   /system/lib/libmdsprpc.so
cp -f /vendor/lib64/libmdsprpc.so /system/lib64/libmdsprpc.so
chmod 644 /system/lib/libmdsprpc.so /system/lib64/libmdsprpc.so
chown root:root /system/lib/libmdsprpc.so /system/lib64/libmdsprpc.so
chcon u:object_r:system_lib_file:s0 /system/lib/libmdsprpc.so /system/lib64/libmdsprpc.so 2>/dev/null
sync
ls -l /system/lib/libmdsprpc.so
echo "--- restart qvrd ---"
stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8
grep VmRSS /proc/$(pidof qvrservice)/status
echo "=== THE TEST ==="
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -12
sleep 1
logcat -d | grep -iE "DspWrapper|MapperWrapper|Tracker|VR mode|Plugin" | tail -10

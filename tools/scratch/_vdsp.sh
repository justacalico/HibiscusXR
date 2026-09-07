mount -o rw,remount /system
# copy the vendor originals over our renamed _system copies
for l in libcdsprpc libadsprpc libsdsprpc; do
  cp -f /vendor/lib/$l.so   /system/lib/$l.so   2>/dev/null
  cp -f /vendor/lib64/$l.so /system/lib64/$l.so 2>/dev/null
done
chmod 644 /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so
chown root:root /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so
chcon u:object_r:system_lib_file:s0 /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so 2>/dev/null
sync
ls -l /system/lib/libcdsprpc.so
echo "--- restart qvrd ---"
stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8
grep VmRSS /proc/$(pidof qvrservice)/status
echo "--- test ---"
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -8
sleep 1
echo "--- qvr log ---"
logcat -d | grep -iE "DspWrapper|QVRServiceTracker|VR mode|Plugin not valid|supportedTracking" | tail -10

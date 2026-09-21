mount -o rw,remount /system
mkdir -p /system/lib/rfsa/adsp
for f in /data/local/tmp/rfsa_*.so; do
  b=$(basename $f); b=${b#rfsa_}
  cp "$f" "/system/lib/rfsa/adsp/$b"
done
chmod 755 /system/lib/rfsa /system/lib/rfsa/adsp
chmod 644 /system/lib/rfsa/adsp/*.so
chown -R root:root /system/lib/rfsa
chcon -R u:object_r:system_file:s0 /system/lib/rfsa 2>/dev/null
sync
echo "--- installed ---"
ls /system/lib/rfsa/adsp/
echo "--- restart qvrd ---"
stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8
grep VmRSS /proc/$(pidof qvrservice)/status
echo "--- THE TEST ---"
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -10
sleep 1
logcat -d | grep -iE "DspWrapper|QVRServiceTracker|VR mode|Plugin" | tail -8
rm -f /data/local/tmp/rfsa_*.so

mount -o rw,remount /system
for f in /data/local/tmp/dsp_lib_*.so; do
  b=$(basename $f); b=${b#dsp_lib_}
  cp "$f" "/system/lib/$b"
  # also under the bare name the qvr stub actually dlopens
  bare=$(echo "$b" | sed "s/_system//")
  cp "$f" "/system/lib/$bare"
done
for f in /data/local/tmp/dsp_lib64_*.so; do
  b=$(basename $f); b=${b#dsp_lib64_}
  cp "$f" "/system/lib64/$b"
  bare=$(echo "$b" | sed "s/_system//")
  cp "$f" "/system/lib64/$bare"
done
chmod 644 /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so
chown root:root /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so
chcon u:object_r:system_lib_file:s0 /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so 2>/dev/null
sync
echo "--- installed ---"
ls -l /system/lib/lib?dsprpc*.so
echo "--- restarting qvrd ---"
stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8
echo "--- rss (stock ~7.4MB when the tracker is up) ---"
grep VmRSS /proc/$(pidof qvrservice)/status
echo "--- QVR now ---"
logcat -d -t 200 | grep -iE "QVRService|DspWrapper|Tracker" | tail -12

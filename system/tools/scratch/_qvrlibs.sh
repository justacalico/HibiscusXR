mount -o rw,remount /system
for f in /data/local/tmp/qvr_lib_*.so;   do b=$(basename $f); b=${b#qvr_lib_};   cp "$f" "/system/lib/$b";   chmod 644 "/system/lib/$b"; done
for f in /data/local/tmp/qvr_lib64_*.so; do b=$(basename $f); b=${b#qvr_lib64_}; cp "$f" "/system/lib64/$b"; chmod 644 "/system/lib64/$b"; done
chown root:root /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null
chcon u:object_r:system_lib_file:s0 /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null
sync
ls -l /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null
echo "--- restarting qvrd ---"
stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 6
getprop init.svc.pn2_qvrd
ps -A | grep -i qvrservice
echo "--- rss (stock is ~7.4MB with plugins) ---"
grep VmRSS /proc/$(pidof qvrservice)/status
echo "--- plugins mapped now? ---"
grep -oE "/system/lib/libqvr[^ ]*\.so|/system/lib/libtobii[^ ]*\.so" /proc/$(pidof qvrservice)/maps | sort -u

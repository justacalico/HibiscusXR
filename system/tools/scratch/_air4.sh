mount -o rw,remount /system
cp /data/local/tmp/libskia_stub.so /system/lib64/pvr_air/libskia.so
chmod 644 /system/lib64/pvr_air/libskia.so
chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libskia.so 2>/dev/null
# retire the 8.1 ICU: Q ICU 63 must serve everyone, and nothing needs 60 now
mv -f /system/lib64/pvr_air/libicuuc.so /system/lib64/pvr_air/libicuuc.so.unused 2>/dev/null
mv -f /system/lib64/pvr_air/libicui18n.so /system/lib64/pvr_air/libicui18n.so.unused 2>/dev/null
sync
ls /system/lib64/pvr_air/
stop airservice
start airservice
sleep 6
echo "--- state ---"
getprop init.svc.airservice
service list 2>/dev/null | grep -i air
ps -A 2>/dev/null | grep -i airservice

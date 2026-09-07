mount -o rw,remount /system
cp /data/local/tmp/txml.so /system/lib64/pvr_air/libtinyxml2.so
chmod 644 /system/lib64/pvr_air/libtinyxml2.so
chown root:root /system/lib64/pvr_air/libtinyxml2.so
chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libtinyxml2.so 2>/dev/null
sync
ls /system/lib64/pvr_air/
echo "--- restarting airservice ---"
stop airservice
start airservice
sleep 4
getprop init.svc.airservice
service list 2>/dev/null | grep -i air
ps -A 2>/dev/null | grep -i airservice

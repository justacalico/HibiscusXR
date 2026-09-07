mount -o rw,remount /system
cp /data/local/tmp/libshim_air.so /system/lib64/pvr_air/libshim_air.so
chmod 644 /system/lib64/pvr_air/libshim_air.so
chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libshim_air.so 2>/dev/null
sync
stop airservice
start airservice
sleep 5
getprop init.svc.airservice
service list 2>/dev/null | grep -i air
ps -A 2>/dev/null | grep -i airservice

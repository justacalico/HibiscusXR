mount -o rw,remount /system
cp /data/local/tmp/libshim_air.so /system/lib64/pvr_air/libshim_air.so
chmod 644 /system/lib64/pvr_air/libshim_air.so
chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libshim_air.so 2>/dev/null
# retire the 8.1 tinyxml2: the shim now supplies the old ctor on top of Q copy
mv -f /system/lib64/pvr_air/libtinyxml2.so /system/lib64/pvr_air/libtinyxml2.so.unused 2>/dev/null
sync
ls /system/lib64/pvr_air/
stop airservice
start airservice
sleep 5
echo "--- state ---"
getprop init.svc.airservice
service list 2>/dev/null | grep -i air
ps -A 2>/dev/null | grep -i airservice

mount -o rw,remount /system
cp /data/local/tmp/libshim_air.so /system/lib64/pvr_air/libshim_air.so
chmod 644 /system/lib64/pvr_air/libshim_air.so
chown root:root /system/lib64/pvr_air/libshim_air.so
chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libshim_air.so 2>/dev/null
cp /data/local/tmp/pn2-airservice.rc /system/etc/init/pn2-airservice.rc
chmod 644 /system/etc/init/pn2-airservice.rc
sync
ls /system/lib64/pvr_air/

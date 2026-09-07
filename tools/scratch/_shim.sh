mount -o rw,remount /system
cat /data/local/tmp/libshim_pvr.so > /system/lib64/libshim_pvr.so
chmod 644 /system/lib64/libshim_pvr.so
chown root:root /system/lib64/libshim_pvr.so
sync
ls -l /system/lib64/libshim_pvr.so
md5sum /system/lib64/libshim_pvr.so /data/local/tmp/libshim_pvr.so

mount -o rw,remount /system
cp /data/local/tmp/db_lib64.so /system/lib64/libdatabuffer.so
cp /data/local/tmp/db_lib.so /system/lib/libdatabuffer.so
chmod 644 /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so
chown root:root /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so
chcon u:object_r:system_file:s0 /system/lib64/libdatabuffer.so 2>/dev/null
chcon u:object_r:system_file:s0 /system/lib/libdatabuffer.so 2>/dev/null
sync
ls -l /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so

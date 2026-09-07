mount -o rw,remount /system
cp /data/local/tmp/pn2-qvrd.rc /system/etc/init/pn2-qvrd.rc
chmod 644 /system/etc/init/pn2-qvrd.rc
sync

mount -o rw,remount /system
cp /system/etc/init/pn2-qvrd.rc /system/etc/init/pn2-qvrd.rc.shell
cp /data/local/tmp/pn2-qvrd.rc /system/etc/init/pn2-qvrd.rc
chmod 644 /system/etc/init/pn2-qvrd.rc
sync
grep seclabel /system/etc/init/pn2-qvrd.rc

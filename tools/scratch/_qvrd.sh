mount -o rw,remount /system
cp /data/local/tmp/pn2-qvrd.rc /system/etc/init/pn2-qvrd.rc
chmod 644 /system/etc/init/pn2-qvrd.rc
# the binary was copied in unlabeled; give it a real context
chcon u:object_r:system_file:s0 /system/bin/qvrservice
ls -lZ /system/bin/qvrservice
sync

#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp /data/local/tmp/pn2-power.rc /system/etc/init/pn2-power.rc
chmod 644 /system/etc/init/pn2-power.rc
chown root:root /system/etc/init/pn2-power.rc
sync
mount -o ro,remount /system
# apply now too, so it is protected before the next reboot
echo pn2_nosuspend > /sys/power/wake_lock
echo "rc installed: $(ls -l /system/etc/init/pn2-power.rc)"
echo "wakelocks now: $(cat /sys/power/wake_lock)"
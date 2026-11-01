mount -o rw,remount /system
cp /data/local/tmp/hibiscus-adbwifi.rc /system/etc/init/hibiscus-adbwifi.rc
chmod 644 /system/etc/init/hibiscus-adbwifi.rc
chown root:root /system/etc/init/hibiscus-adbwifi.rc
sync
ls -l /system/etc/init/hibiscus-adbwifi.rc
echo "--- enabling ---"
setprop persist.hibiscus.adbwifi 1
sleep 3
echo "service.adb.tcp.port = $(getprop service.adb.tcp.port)"
echo "persist.hibiscus.adbwifi = $(getprop persist.hibiscus.adbwifi)"

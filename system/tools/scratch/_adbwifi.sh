mount -o rw,remount /system
cp /data/local/tmp/pn2-adbwifi.rc /system/etc/init/pn2-adbwifi.rc
chmod 644 /system/etc/init/pn2-adbwifi.rc
chown root:root /system/etc/init/pn2-adbwifi.rc
sync
ls -l /system/etc/init/pn2-adbwifi.rc
echo "--- enabling ---"
setprop persist.pn2.adbwifi 1
sleep 3
echo "service.adb.tcp.port = $(getprop service.adb.tcp.port)"
echo "persist.pn2.adbwifi  = $(getprop persist.pn2.adbwifi)"

#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
rm -rf /system/priv-app/provision2d
cp -r /data/local/tmp/oemapps/provision2d /system/priv-app/provision2d
chown -R root:root /system/priv-app/provision2d
find /system/priv-app/provision2d -type d -exec chmod 755 {} \;
find /system/priv-app/provision2d -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/PVRHome
cp -r /data/local/tmp/oemapps/PVRHome /system/priv-app/PVRHome
chown -R root:root /system/priv-app/PVRHome
find /system/priv-app/PVRHome -type d -exec chmod 755 {} \;
find /system/priv-app/PVRHome -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/PVRLauncher
cp -r /data/local/tmp/oemapps/PVRLauncher /system/priv-app/PVRLauncher
chown -R root:root /system/priv-app/PVRLauncher
find /system/priv-app/PVRLauncher -type d -exec chmod 755 {} \;
find /system/priv-app/PVRLauncher -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/store2d
cp -r /data/local/tmp/oemapps/store2d /system/priv-app/store2d
chown -R root:root /system/priv-app/store2d
find /system/priv-app/store2d -type d -exec chmod 755 {} \;
find /system/priv-app/store2d -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/ToBToolService
cp -r /data/local/tmp/oemapps/ToBToolService /system/priv-app/ToBToolService
chown -R root:root /system/priv-app/ToBToolService
find /system/priv-app/ToBToolService -type d -exec chmod 755 {} \;
find /system/priv-app/ToBToolService -type f -exec chmod 644 {} \;
sync
mount -o ro,remount /system
echo "--- installed ---"
for n in PVRLauncher PVRHome store2d provision2d ToBToolService; do
  [ -d /system/priv-app/$n ] && echo "  $n: $(ls /system/priv-app/$n/*.apk 2>/dev/null | wc -l) apk"
done

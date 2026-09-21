#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
rm -rf /system/priv-app/CVService/lib
cp -r /data/local/tmp/applibs/CVService/lib /system/priv-app/CVService/lib
chown -R root:root /system/priv-app/CVService/lib
find /system/priv-app/CVService/lib -type d -exec chmod 755 {} \;
find /system/priv-app/CVService/lib -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/InitServer/lib
cp -r /data/local/tmp/applibs/InitServer/lib /system/priv-app/InitServer/lib
chown -R root:root /system/priv-app/InitServer/lib
find /system/priv-app/InitServer/lib -type d -exec chmod 755 {} \;
find /system/priv-app/InitServer/lib -type f -exec chmod 644 {} \;
rm -rf /system/app/PicoToSvrService/lib
cp -r /data/local/tmp/applibs/PicoToSvrService/lib /system/app/PicoToSvrService/lib
chown -R root:root /system/app/PicoToSvrService/lib
find /system/app/PicoToSvrService/lib -type d -exec chmod 755 {} \;
find /system/app/PicoToSvrService/lib -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/PVRVerify/lib
cp -r /data/local/tmp/applibs/PVRVerify/lib /system/priv-app/PVRVerify/lib
chown -R root:root /system/priv-app/PVRVerify/lib
find /system/priv-app/PVRVerify/lib -type d -exec chmod 755 {} \;
find /system/priv-app/PVRVerify/lib -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/VRShell2/lib
cp -r /data/local/tmp/applibs/VRShell2/lib /system/priv-app/VRShell2/lib
chown -R root:root /system/priv-app/VRShell2/lib
find /system/priv-app/VRShell2/lib -type d -exec chmod 755 {} \;
find /system/priv-app/VRShell2/lib -type f -exec chmod 644 {} \;
rm -rf /system/priv-app/VRUserCenter2/lib
cp -r /data/local/tmp/applibs/VRUserCenter2/lib /system/priv-app/VRUserCenter2/lib
chown -R root:root /system/priv-app/VRUserCenter2/lib
find /system/priv-app/VRUserCenter2/lib -type d -exec chmod 755 {} \;
find /system/priv-app/VRUserCenter2/lib -type f -exec chmod 644 {} \;
for a in lib64 lib; do
  if [ -f /system/$a/libPvr_UnitySDK.so.orig ]; then
    cp -f /system/$a/libPvr_UnitySDK.so.orig /system/$a/libPvr_UnitySDK.so
    rm -f /system/$a/libPvr_UnitySDK.so.orig
    echo "reverted /system/$a/libPvr_UnitySDK.so to stock"
  fi
done
sync
mount -o ro,remount /system
echo "--- app libs installed ---"
for d in /system/priv-app/*/lib /system/app/*/lib; do
  [ -d "$d" ] && echo "$d: $(find $d -name "*.so" | wc -l) libs"
done

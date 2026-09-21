#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
rm -rf /system/priv-app/configserverservice
mkdir -p /system/priv-app/configserverservice
cp /data/local/tmp/pvrfinal/configserverservice.apk /system/priv-app/configserverservice/configserverservice.apk
chmod 755 /system/priv-app/configserverservice
chmod 644 /system/priv-app/configserverservice/configserverservice.apk
chown -R root:root /system/priv-app/configserverservice
rm -rf /system/priv-app/CVService
mkdir -p /system/priv-app/CVService
cp /data/local/tmp/pvrfinal/CVService.apk /system/priv-app/CVService/CVService.apk
chmod 755 /system/priv-app/CVService
chmod 644 /system/priv-app/CVService/CVService.apk
chown -R root:root /system/priv-app/CVService
rm -rf /system/priv-app/InitServer
mkdir -p /system/priv-app/InitServer
cp /data/local/tmp/pvrfinal/InitServer.apk /system/priv-app/InitServer/InitServer.apk
chmod 755 /system/priv-app/InitServer
chmod 644 /system/priv-app/InitServer/InitServer.apk
chown -R root:root /system/priv-app/InitServer
rm -rf /system/priv-app/PicoSettingsProvider
mkdir -p /system/priv-app/PicoSettingsProvider
cp /data/local/tmp/pvrfinal/PicoSettingsProvider.apk /system/priv-app/PicoSettingsProvider/PicoSettingsProvider.apk
chmod 755 /system/priv-app/PicoSettingsProvider
chmod 644 /system/priv-app/PicoSettingsProvider/PicoSettingsProvider.apk
chown -R root:root /system/priv-app/PicoSettingsProvider
rm -rf /system/app/PicoToSvrService
mkdir -p /system/app/PicoToSvrService
cp /data/local/tmp/pvrfinal/PicoToSvrService.apk /system/app/PicoToSvrService/PicoToSvrService.apk
chmod 755 /system/app/PicoToSvrService
chmod 644 /system/app/PicoToSvrService/PicoToSvrService.apk
chown -R root:root /system/app/PicoToSvrService
rm -rf /system/priv-app/pvrdisplay
mkdir -p /system/priv-app/pvrdisplay
cp /data/local/tmp/pvrfinal/pvrdisplay.apk /system/priv-app/pvrdisplay/pvrdisplay.apk
chmod 755 /system/priv-app/pvrdisplay
chmod 644 /system/priv-app/pvrdisplay/pvrdisplay.apk
chown -R root:root /system/priv-app/pvrdisplay
rm -rf /system/priv-app/PVRVerify
mkdir -p /system/priv-app/PVRVerify
cp /data/local/tmp/pvrfinal/PVRVerify.apk /system/priv-app/PVRVerify/PVRVerify.apk
chmod 755 /system/priv-app/PVRVerify
chmod 644 /system/priv-app/PVRVerify/PVRVerify.apk
chown -R root:root /system/priv-app/PVRVerify
rm -rf /system/priv-app/pvr_adapter
mkdir -p /system/priv-app/pvr_adapter
cp /data/local/tmp/pvrfinal/pvr_adapter.apk /system/priv-app/pvr_adapter/pvr_adapter.apk
chmod 755 /system/priv-app/pvr_adapter
chmod 644 /system/priv-app/pvr_adapter/pvr_adapter.apk
chown -R root:root /system/priv-app/pvr_adapter
rm -rf /system/priv-app/ShortcutMenu
mkdir -p /system/priv-app/ShortcutMenu
cp /data/local/tmp/pvrfinal/ShortcutMenu.apk /system/priv-app/ShortcutMenu/ShortcutMenu.apk
chmod 755 /system/priv-app/ShortcutMenu
chmod 644 /system/priv-app/ShortcutMenu/ShortcutMenu.apk
chown -R root:root /system/priv-app/ShortcutMenu
rm -rf /system/priv-app/VRShell2
mkdir -p /system/priv-app/VRShell2
cp /data/local/tmp/pvrfinal/VRShell2.apk /system/priv-app/VRShell2/VRShell2.apk
chmod 755 /system/priv-app/VRShell2
chmod 644 /system/priv-app/VRShell2/VRShell2.apk
chown -R root:root /system/priv-app/VRShell2
rm -rf /system/priv-app/VRUserCenter2
mkdir -p /system/priv-app/VRUserCenter2
cp /data/local/tmp/pvrfinal/VRUserCenter2.apk /system/priv-app/VRUserCenter2/VRUserCenter2.apk
chmod 755 /system/priv-app/VRUserCenter2
chmod 644 /system/priv-app/VRUserCenter2/VRUserCenter2.apk
chown -R root:root /system/priv-app/VRUserCenter2
sync
mount -o ro,remount /system
echo "--- installed ---"
ls -d /system/priv-app/*/ /system/app/*/ 2>/dev/null | grep -iE "pvr|pico|CVService|ShortcutMenu|InitServer|configserver"

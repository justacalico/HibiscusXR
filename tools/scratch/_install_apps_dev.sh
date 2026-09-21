#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
mkdir -p /system/priv-app/configserverservice
cp /data/local/tmp/pvrapps/configserverservice.apk /system/priv-app/configserverservice/configserverservice.apk
chmod 755 /system/priv-app/configserverservice
chmod 644 /system/priv-app/configserverservice/configserverservice.apk
chown -R root:root /system/priv-app/configserverservice
mkdir -p /system/priv-app/CVService
cp /data/local/tmp/pvrapps/CVService.apk /system/priv-app/CVService/CVService.apk
chmod 755 /system/priv-app/CVService
chmod 644 /system/priv-app/CVService/CVService.apk
chown -R root:root /system/priv-app/CVService
mkdir -p /system/priv-app/InitServer
cp /data/local/tmp/pvrapps/InitServer.apk /system/priv-app/InitServer/InitServer.apk
chmod 755 /system/priv-app/InitServer
chmod 644 /system/priv-app/InitServer/InitServer.apk
chown -R root:root /system/priv-app/InitServer
mkdir -p /system/priv-app/PicoSettingsProvider
cp /data/local/tmp/pvrapps/PicoSettingsProvider.apk /system/priv-app/PicoSettingsProvider/PicoSettingsProvider.apk
chmod 755 /system/priv-app/PicoSettingsProvider
chmod 644 /system/priv-app/PicoSettingsProvider/PicoSettingsProvider.apk
chown -R root:root /system/priv-app/PicoSettingsProvider
mkdir -p /system/app/PicoToSvrService
cp /data/local/tmp/pvrapps/PicoToSvrService.apk /system/app/PicoToSvrService/PicoToSvrService.apk
chmod 755 /system/app/PicoToSvrService
chmod 644 /system/app/PicoToSvrService/PicoToSvrService.apk
chown -R root:root /system/app/PicoToSvrService
mkdir -p /system/priv-app/pvrdisplay
cp /data/local/tmp/pvrapps/pvrdisplay.apk /system/priv-app/pvrdisplay/pvrdisplay.apk
chmod 755 /system/priv-app/pvrdisplay
chmod 644 /system/priv-app/pvrdisplay/pvrdisplay.apk
chown -R root:root /system/priv-app/pvrdisplay
mkdir -p /system/priv-app/PVRVerify
cp /data/local/tmp/pvrapps/PVRVerify.apk /system/priv-app/PVRVerify/PVRVerify.apk
chmod 755 /system/priv-app/PVRVerify
chmod 644 /system/priv-app/PVRVerify/PVRVerify.apk
chown -R root:root /system/priv-app/PVRVerify
mkdir -p /system/priv-app/pvr_adapter
cp /data/local/tmp/pvrapps/pvr_adapter.apk /system/priv-app/pvr_adapter/pvr_adapter.apk
chmod 755 /system/priv-app/pvr_adapter
chmod 644 /system/priv-app/pvr_adapter/pvr_adapter.apk
chown -R root:root /system/priv-app/pvr_adapter
mkdir -p /system/app/PxrNotification
cp /data/local/tmp/pvrapps/PxrNotification.apk /system/app/PxrNotification/PxrNotification.apk
chmod 755 /system/app/PxrNotification
chmod 644 /system/app/PxrNotification/PxrNotification.apk
chown -R root:root /system/app/PxrNotification
mkdir -p /system/priv-app/ShortcutMenu
cp /data/local/tmp/pvrapps/ShortcutMenu.apk /system/priv-app/ShortcutMenu/ShortcutMenu.apk
chmod 755 /system/priv-app/ShortcutMenu
chmod 644 /system/priv-app/ShortcutMenu/ShortcutMenu.apk
chown -R root:root /system/priv-app/ShortcutMenu
mkdir -p /system/priv-app/VRShell2
cp /data/local/tmp/pvrapps/VRShell2.apk /system/priv-app/VRShell2/VRShell2.apk
chmod 755 /system/priv-app/VRShell2
chmod 644 /system/priv-app/VRShell2/VRShell2.apk
chown -R root:root /system/priv-app/VRShell2
mkdir -p /system/priv-app/VRUserCenter2
cp /data/local/tmp/pvrapps/VRUserCenter2.apk /system/priv-app/VRUserCenter2/VRUserCenter2.apk
chmod 755 /system/priv-app/VRUserCenter2
chmod 644 /system/priv-app/VRUserCenter2/VRUserCenter2.apk
chown -R root:root /system/priv-app/VRUserCenter2
grep -q "^ro.control_privapp_permissions" /system/build.prop || {
  echo "" >> /system/build.prop
  echo "# Pico apps ship no privapp-permissions allowlist (8.1 only warned)." >> /system/build.prop
  echo "ro.control_privapp_permissions=log" >> /system/build.prop
}
sync
mount -o ro,remount /system
echo "--- installed ---"
ls -d /system/priv-app/*/ /system/app/*/ 2>/dev/null | grep -iE "pvr|pico|vr|CVService|ShortcutMenu|InitServer|configserver"
echo "privapp mode: $(getprop ro.control_privapp_permissions)"

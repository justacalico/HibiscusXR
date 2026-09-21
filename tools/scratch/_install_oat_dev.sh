#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
rm -rf /system/priv-app/configserverservice
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/configserverservice /system/priv-app/configserverservice
rm -rf /system/priv-app/CVService
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/CVService /system/priv-app/CVService
rm -rf /system/priv-app/InitServer
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/InitServer /system/priv-app/InitServer
rm -rf /system/priv-app/PicoSettingsProvider
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/PicoSettingsProvider /system/priv-app/PicoSettingsProvider
rm -rf /system/app/PicoToSvrService
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/PicoToSvrService /system/app/PicoToSvrService
rm -rf /system/priv-app/pvrdisplay
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/pvrdisplay /system/priv-app/pvrdisplay
rm -rf /system/priv-app/PVRVerify
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/PVRVerify /system/priv-app/PVRVerify
rm -rf /system/priv-app/pvr_adapter
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/pvr_adapter /system/priv-app/pvr_adapter
rm -rf /system/app/PxrNotification
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/PxrNotification /system/app/PxrNotification
rm -rf /system/priv-app/ShortcutMenu
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/ShortcutMenu /system/priv-app/ShortcutMenu
rm -rf /system/priv-app/VRShell2
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/VRShell2 /system/priv-app/VRShell2
rm -rf /system/priv-app/VRUserCenter2
cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/VRUserCenter2 /system/priv-app/VRUserCenter2
for w in priv-app app; do
  for d in configserverservice CVService InitServer PicoSettingsProvider pvrdisplay PVRVerify pvr_adapter ShortcutMenu VRShell2 VRUserCenter2 PicoToSvrService PxrNotification; do
    [ -d /system/$w/$d ] || continue
    chown -R root:root /system/$w/$d
    find /system/$w/$d -type d -exec chmod 755 {} \;
    find /system/$w/$d -type f -exec chmod 644 {} \;
  done
done
sync
mount -o ro,remount /system
echo "--- installed, with oat ---"
for w in priv-app app; do
  for d in /system/$w/*/; do
    case "$d" in *VRShell2*|*pvrdisplay*|*CVService*|*PxrNotification*|*pvr_adapter*|*configserver*|*PicoSettings*|*PVRVerify*|*ShortcutMenu*|*InitServer*|*VRUserCenter2*|*PicoToSvr*)
      echo "$d  apk=$(ls $d*.apk 2>/dev/null | wc -l)  oat=$(find $d/oat -type f 2>/dev/null | wc -l)" ;;
    esac
  done
done

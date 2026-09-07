#!/system/bin/sh
# Back out the Pico apps. They were installed without their oat/ directories, so
# they have no bytecode at all - and com.pvr.pxrnotification is a PERSISTENT
# process, so ActivityManager restarts it several times a second forever at
# system uid. That is what locked the device up.
exec 2>&1
mount -o rw,remount /system
for d in configserverservice CVService InitServer PicoSettingsProvider pvrdisplay \
         PVRVerify pvr_adapter ShortcutMenu VRShell2 VRUserCenter2; do
  rm -rf /system/priv-app/$d
done
for d in PicoToSvrService PxrNotification; do
  rm -rf /system/app/$d
done
sync
mount -o ro,remount /system
echo "=== remaining pico dirs (should be none) ==="
ls -d /system/priv-app/*/ /system/app/*/ 2>/dev/null | grep -iE 'pvr|pico|CVService|ShortcutMenu|InitServer|configserver|PxrNotif' || echo "  clean"
echo DONE

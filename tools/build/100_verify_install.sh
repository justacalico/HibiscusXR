#!/system/bin/sh
n=0
for d in configserverservice CVService InitServer PicoSettingsProvider pvrdisplay \
         PVRVerify pvr_adapter ShortcutMenu VRShell2 VRUserCenter2; do
  if [ -f /system/priv-app/$d/$d.apk ]; then
    echo "OK   priv-app/$d  $(stat -c%s /system/priv-app/$d/$d.apk)"
    n=$((n+1))
  else
    echo "MISS priv-app/$d"
  fi
done
if [ -f /system/app/PicoToSvrService/PicoToSvrService.apk ]; then
  echo "OK   app/PicoToSvrService"
  n=$((n+1))
else
  echo "MISS app/PicoToSvrService"
fi
echo "installed: $n / 11"
echo "no stale oat dirs? $(find /system/priv-app /system/app -name oat -path '*VRShell*' 2>/dev/null | wc -l) (want 0)"

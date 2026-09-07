#!/system/bin/sh
# Why does PackageManager register nothing from /oem on Android 10, when the
# same partition's apps are all registered on stock 8.1?
#
# AOSP PMS scans Environment.getOemDirectory()/app and /priv-app, so either the
# scan is rejecting them, or /oem was not mounted yet when the scan ran.
exec 2>&1
echo "##### /oem mount and contents #####"
grep -w /oem /proc/mounts
ls -l /oem/priv-app/ 2>&1
echo
echo "##### are the apks actually there and readable #####"
for d in /oem/priv-app/*/; do
  n=$(basename "$d")
  a=$(ls "$d"*.apk 2>/dev/null | head -1)
  if [ -n "$a" ]; then
    echo "  $n -> $(stat -c%s "$a") bytes, mode $(stat -c%a "$a"), owner $(stat -c%U:%G "$a")"
  else
    echo "  $n -> NO APK"
  fi
done
echo
echo "##### does PM know any of them #####"
pm list packages 2>/dev/null | grep -icE 'pvr\.home|pvr\.launcher|picovr\.store|picovr\.provision|tobservice'
echo "  ^ count of oem packages registered (0 = none)"
echo
echo "##### PackageManager complaints mentioning oem #####"
logcat -d -b all 2>/dev/null | grep -iE 'oem' | grep -iE 'packagemanager|scan|skip|reject' | tail -20
echo
echo "##### any scan errors at all #####"
logcat -d -b all 2>/dev/null | grep -iE 'Failed to scan|Package .* ignored|No package|scanning' | tail -20
echo DONE

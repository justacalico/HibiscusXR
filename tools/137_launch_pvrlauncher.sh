#!/system/bin/sh
# 1. CVService is crash-looping several times a second: PackageManager recorded a
#    64-bit ABI for it (the apk had no lib/ when first scanned), so it looks in
#    lib/arm64 while its libraries are 32-bit in lib/arm. Disable it - it is the
#    controller service and not needed to bring the launcher up. Fixing the ABI
#    properly means making PM re-derive it, which is a separate job.
#
# 2. com.pvr.launcher declares category HOME, so start it as the home activity.
exec 2>&1
echo "=== stopping the CVService crash loop ==="
pm disable com.picovr.picovrlib.cvcontroller
sleep 1

echo
echo "=== launching com.pvr.launcher via HOME ==="
logcat -c
am start -a android.intent.action.MAIN -c android.intent.category.HOME -n com.pvr.launcher/.MainActivity 2>&1
sleep 2
# if that exact activity name is wrong, let the system resolve it
if [ -z "$(pidof com.pvr.launcher)" ]; then
  echo "  (explicit component failed, trying resolver)"
  am start -a android.intent.action.MAIN -c android.intent.category.HOME -p com.pvr.launcher 2>&1
  sleep 3
fi
echo
echo "pid: $(pidof com.pvr.launcher)"
dumpsys activity activities | grep -m1 mResumedActivity
echo
echo "=== log ==="
logcat -d | grep -iE 'pvr.launcher|PVRLauncher|AndroidRuntime|SIGSEGV|Displayed' | tail -12
echo DONE

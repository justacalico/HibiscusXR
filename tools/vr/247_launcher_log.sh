#!/system/bin/sh
P=$(pidof com.pvr.launcher)
echo "=== everything from pid $P ==="
logcat -d 2>/dev/null | grep " $P " | tail -40
echo
echo "=== window / surface state - is it actually drawing? ==="
dumpsys window windows 2>/dev/null | grep -iE 'Window #|mCurrentFocus|mFocusedApp|com.pvr' | head -20
echo
echo "=== is it a unity app at all? ==="
ls /system/priv-app/PVRLauncher/ 2>/dev/null
ls /system/priv-app/PVRLauncher/lib/* 2>/dev/null

#!/system/bin/sh
echo "=== which app_process runs the controller service? ==="
for P in $(pidof com.picovr.picovrlib.cvcontroller:RemoteService) $(pidof com.picovr.picovrlib.cvcontroller); do
  printf 'pid %-7s ' "$P"
  readlink /proc/$P/exe 2>/dev/null
done
echo
echo "=== ABIs inside the APK ==="
unzip -l /system/priv-app/CVService/CVService.apk 2>/dev/null | grep -E "lib/" | head -20
echo
echo "=== what lib dirs exist on disk ==="
ls -l /system/priv-app/CVService/lib/ 2>/dev/null
echo
echo "=== what PM decided ==="
dumpsys package com.picovr.picovrlib.cvcontroller 2>/dev/null | grep -iE "primaryCpuAbi|secondaryCpuAbi|legacyNativeLibraryDir|codePath"

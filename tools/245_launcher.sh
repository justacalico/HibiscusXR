#!/system/bin/sh
# Stock's home is com.pvr.launcher/.MainActivity. Ours has the package but no
# activity resolves. Is it enabled, does it have code, and where does it live?
for P in com.pvr.launcher com.pvr.home com.pvr.vrshell; do
  echo "######## $P"
  dumpsys package $P 2>/dev/null | grep -iE 'codePath|versionName|primaryCpuAbi|enabled=|pkgFlags|dataDir' | head -6
  echo "--- activities with HOME/LAUNCHER ---"
  dumpsys package $P 2>/dev/null | grep -B2 -A6 'android.intent.category.HOME' | head -12
  echo
done
echo "=== does the launcher apk actually contain dex? ==="
for D in /system/app /system/priv-app; do
  for N in PicoLauncher pvrlauncher launcher Launcher home Home; do
    [ -d "$D/$N" ] && echo "$D/$N:" && ls -l "$D/$N"
  done
done 2>/dev/null

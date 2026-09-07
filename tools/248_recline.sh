#!/system/bin/sh
# Recline mode defaults to 3DoF and skips the SLAM calibration the seethrough app
# otherwise forces at boot. That is a way to a working home shell without the
# camera path, which is blocked on the 8.1-binary-vs-Q-camera-HAL problem.
echo "=== settings mentioning recline / 3dof / calibration ==="
for ns in system secure global; do
  settings list $ns 2>/dev/null | grep -iE 'recline|3dof|6dof|calib|slam|seethrough|tracking'
done
echo
echo "=== props ==="
getprop 2>/dev/null | grep -iE 'recline|3dof|6dof|calib|slam|tracking_mode'
echo
echo "=== pico config files that might hold it ==="
for f in /data/local/pvr* /sdcard/psmart/* /data/misc/pvr* ; do
  [ -f "$f" ] && echo "--- $f ---" && head -20 "$f"
done 2>/dev/null
echo
echo "=== is there an /oem partition, and provision2d on it? ==="
ls -d /oem 2>/dev/null && ls /oem/priv-app/ 2>/dev/null | head -20
echo "provision2d:"
ls -l /oem/priv-app/provision2d/ 2>/dev/null

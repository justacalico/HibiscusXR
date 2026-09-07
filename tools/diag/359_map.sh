#!/system/bin/sh
# "Pose quality 0.00 < 0.70, relocation progress" + "6DOF map was not saved":
# the SLAM tracker is trying to relocalise against a map that does not exist.
# Find where the map/calibration lives so we can either build one or bypass it.
echo "=== qvr / tracking data directories ==="
for d in /data/misc/qvr /data/misc/qvrservice /data/vendor/qvr /persist/qvr \
         /data/misc/pvr /data/pvr /persist/pvr /data/local/pvr /sdcard/psmart; do
  [ -e "$d" ] && { echo "  $d:"; ls -l "$d" 2>/dev/null | head -8; }
done
echo
echo "=== anything that looks like a map ==="
find /data /persist -iname "*6dof*" -o -iname "*.map" -o -iname "*slam*" -o -iname "*reloc*" 2>/dev/null | head -20
echo
echo "=== calibration files ==="
find /persist /data/misc -iname "*calib*" 2>/dev/null | head -20
echo
echo "=== where does qvrservice say it saves maps ==="
strings -a /system/bin/qvrservice 2>/dev/null | grep -iE "^/(data|persist)[a-z0-9/_.-]*$" | sort -u | head -20
echo
echo "=== seethrough / calibration apps present ==="
pm list packages 2>/dev/null | grep -iE "seethrough|calib|provision|initserver"
echo
echo "=== the seethrough switch ==="
getprop persist.pvrcon.seethrough.enable

#!/system/bin/sh
# Our unit never had the .merge files (confirmed from its own pre-conversion
# persist backup) and ran fine on stock, so those are Eye-model extras.
# The real gap is /data/misc/qvr/calibration_data.bin, which qvrservice generates
# from the factory calibration. Find out what it reads and why ours never appears.
echo "=== which library owns the calibration path ==="
for f in /vendor/lib64/libqvr*.so /vendor/lib/libqvr*.so /system/lib64/libqvr*.so; do
  if strings -a "$f" 2>/dev/null | grep -q "calibration_data.bin"; then
    echo "  $f"
    strings -a "$f" 2>/dev/null | grep -iE "calibration|device_calib|\.bin" | head -12
  fi
done
echo
echo "=== restart qvrservice and watch it look for calibration ==="
logcat -c
stop pn2_qvrd 2>/dev/null; stop qvrd 2>/dev/null
sleep 2
start pn2_qvrd 2>/dev/null || start qvrd 2>/dev/null
sleep 8
logcat -d | grep -iE "calib|QVRServiceTracker|eeprom|extrinsic|intrinsic" | grep -viE "LockBuffer|sensors-hal" | head -25
echo
echo "=== did it create anything? ==="
ls -l /data/misc/qvr/
echo
echo "=== permissions on the dir (can it write?) ==="
ls -ld /data/misc/qvr
id

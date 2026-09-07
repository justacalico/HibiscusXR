#!/system/bin/sh
# calibration_data.bin is per-device and derived from the factory calibration in
# /persist, which survives a data wipe. Find out whether OUR headset still has its
# own factory calibration - if it does, this is regenerable rather than lost.
echo "=== /persist/pvr/camera ==="
ls -l /persist/pvr/camera/ 2>/dev/null
echo
echo "=== /persist/pvr/lens ==="
ls -l /persist/pvr/lens/ 2>/dev/null
echo
echo "=== /persist/calibrationfiles ==="
ls -lR /persist/calibrationfiles/ 2>/dev/null | head -20
echo
echo "=== /persist/calibration ==="
ls -lR /persist/calibration/ 2>/dev/null | head -20
echo
echo "=== device_calibration.xml: real content or placeholder? ==="
wc -c /persist/pvr/camera/device_calibration.xml 2>/dev/null
head -c 300 /persist/pvr/camera/device_calibration.xml 2>/dev/null; echo
echo
echo "=== what qvrservice looks for ==="
strings -a /system/bin/qvrservice 2>/dev/null | grep -iE "calibration|hwcalib|warm_start" | head -15
echo
echo "=== does anything log a calibration failure? ==="
logcat -d 2>/dev/null | grep -iE "calibration|calib" | grep -viE "PvrOrientationTracker" | tail -12

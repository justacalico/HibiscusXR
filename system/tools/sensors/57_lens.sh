#!/system/bin/sh
# Hunt for eyepiece/lens intrinsics. Camera intrinsics we already have; what we
# need is the display-side distortion polynomial and per-eye projection.
echo "=== current tuning props ==="
for p in roll k1 k2 ipd fov sensor; do echo "debug.pn2vr.$p = $(getprop debug.pn2vr.$p)"; done
echo

echo "=== /vendor/etc/pvr ==="
ls -laR /vendor/etc/pvr/ 2>/dev/null
echo
echo "=== /vendor/etc/qvr ==="
ls -laR /vendor/etc/qvr/ 2>/dev/null
echo

echo "=== any file mentioning lens/distortion/eye geometry ==="
grep -rliE 'distortion|lens|eye_?relief|ipd|interpupil|k1|fov' \
     /vendor/etc/pvr /vendor/etc/qvr /vendor/etc/sensors 2>/dev/null | head -20
echo

echo "=== persist calibration (camera today; check for display blocks) ==="
ls -la /persist/pvr/ 2>/dev/null
echo "--- device_calibration.xml ---"
cat /persist/pvr/camera/device_calibration.xml 2>/dev/null
echo

echo "=== anything else under /persist that looks like calibration ==="
find /persist -maxdepth 3 -type f 2>/dev/null | head -40
echo DONE

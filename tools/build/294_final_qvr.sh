#!/system/bin/sh
# Moment of truth: libcdsprpc.so is in /system and the DSP-side skels are in
# /system/lib/rfsa/adsp. Does the tracker initialise now?
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -10
echo
echo "=== qvr log ==="
logcat -d 2>/dev/null | grep -iE 'DspWrapper|QVRServiceTracker|VR mode|Plugin|remote handle|skel' | tail -12

#!/system/bin/sh
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 >/dev/null 2>&1
sleep 2
echo "=== DSP wrapper / tracker after installing the rpc libs ==="
logcat -d 2>/dev/null | grep -iE 'DspWrapper|QVRServiceTracker|VR mode|Plugin not valid|dlopen' | tail -12
echo
echo "=== does it still say not supported? ==="
timeout 12 /vendor/bin/qvrservicetest64 2>&1 | grep -iE 'supported|version|mode' | head -6

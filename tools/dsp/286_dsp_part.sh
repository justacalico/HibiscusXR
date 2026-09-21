#!/system/bin/sh
# Stock's QVR reports "starting VR mode"; ours "VR not supported". The DSP-side
# skel library must be found for FastRPC to return a remote handle, and on this
# platform those live on the /dsp partition.
echo "=== is /dsp mounted? ==="
mount 2>/dev/null | grep -i dsp
echo "--- contents ---"
ls /dsp 2>/dev/null | head -20
echo "--- looking for qvr skel ---"
find /dsp -iname '*qvr*' 2>/dev/null | head -10
find /dsp -iname '*skel*' 2>/dev/null | head -10
echo
echo "=== the partition itself ==="
ls -l /dev/block/bootdevice/by-name/ 2>/dev/null | grep -iE 'dsp'
echo
echo "=== what ADSP_LIBRARY_PATH would default to ==="
getprop | grep -iE 'adsp|dsp' | head
echo
echo "=== is anything else mounted from sde9 ==="
mount 2>/dev/null | grep -i sde9

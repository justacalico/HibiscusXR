#!/system/bin/sh
# The stub asks the DSP for "qvr_dsp_driver_skel". FastRPC loads that ON the DSP
# from a filesystem path. Does the file exist anywhere?
echo "=== anything named qvr*dsp* or *qvr*skel* ==="
find /vendor /dsp /system /odm -iname '*qvr*' 2>/dev/null | grep -iE 'dsp|skel' | head -10
echo
echo "=== all skel libraries present ==="
find /dsp -name '*_skel.so' 2>/dev/null | sort
echo
echo "=== stock has the same set? compare counts ==="
find /dsp -name '*_skel.so' 2>/dev/null | wc -l
echo
echo "=== is the cdsp firmware image itself present ==="
ls -l /vendor/firmware*/cdsp* /vendor/firmware*/*turing* 2>/dev/null | head
ls /vendor/firmware_mnt 2>/dev/null | grep -iE 'cdsp|turing' | head

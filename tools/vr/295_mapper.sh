#!/system/bin/sh
# The DSP driver now loads (we get a version string back from the DSP itself).
# Next failure: "Initialize mDSP failed: mapper wrapper is NULL" - the mapper
# wrapper, i.e. libqvr_mapper_stub.so -> libqvr_mapper_skel.so.
logcat -c
timeout 12 /vendor/bin/qvrservicetest64 >/dev/null 2>&1
sleep 2
echo "=== every qvr line, unfiltered, from that run ==="
logcat -d 2>/dev/null | grep -iE 'QVR|dsp|mapper' | tail -30
echo
echo "=== mapper stub present + what it needs ==="
ls -l /system/lib/libqvr_mapper_stub.so /system/lib/rfsa/adsp/libqvr_mapper_skel.so 2>/dev/null
echo "--- strings: what does the stub dlopen? ---"
strings -a /system/lib/libqvr_mapper_stub.so 2>/dev/null | grep -iE '\.so|skel|_URI' | sort -u | head

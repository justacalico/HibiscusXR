#!/bin/bash
# Read the crashing code path out of the real libPvr_UnitySDK.so that ALVR ships.
#
# Every Pico VR app dies at:
#   psmvr_UpdateLensAndDisplayInfoFromVRService -> UpdateLensInfo
#   SIGSEGV, fault addr 0x10, sp=0, in art_quick_generic_jni_trampoline
#
# sp=0 inside the generic JNI trampoline means a Java `native` method was invoked
# but never resolved. So the question is: which Java class/method does the SDK
# reach for at that point, and is it present on our build?
set -u
SO=/mnt/f/PN2Lineage/ref/alvr-pico-legacy/app/src/main/jniLibs/armeabi-v7a/libPvr_UnitySDK.so
LOG=/mnt/f/PN2Lineage/notes/154_sdk_symbols.txt
exec >"$LOG" 2>&1

ls -l "$SO"
echo

echo "=== exported symbols matching the crash frames ==="
nm -D --defined-only "$SO" 2>/dev/null | grep -iE 'lens|displayinfo|vrservice' | sort

echo
echo "=== UNDEFINED symbols (what it imports) matching JNI ==="
nm -D -u "$SO" 2>/dev/null | grep -iE 'jni|Java' | head -20

echo
echo "=== every Java class name referenced as a string ==="
strings -a "$SO" | grep -E '^[a-z]+(/[A-Za-z0-9_$]+)+$' | sort -u | head -60

echo
echo "=== psmart / pvr / vrlib strings ==="
strings -a "$SO" | grep -iE 'psmart|vrlib|pvrclient|VrActivity|PvrService' | sort -u | head -60

echo
echo "=== JNI signature strings (method descriptors) ==="
strings -a "$SO" | grep -E '^\(.*\).*$' | grep -E '[;IVFZJ)]' | sort -u | head -60

echo
echo "=== the diagnostic messages ALVR saw ==="
strings -a "$SO" | grep -iE 'env is null|obj_pvr|is null|not found|failed' | sort -u | head -40
echo DONE

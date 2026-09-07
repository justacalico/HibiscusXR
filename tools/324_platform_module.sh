#!/system/bin/sh
# trackingstate comes from _trackingDataExt +0x9c, which pvrservice fills. It is 0,
# i.e. pvrservice reports "not tracking" - because it never talks to QVR.
# libpvrmodule_platform.so is what references libqvrservice_client.so (59 refs).
# Is that module even loaded, and does the dlopen fail?
P=$(pidof pvrservice)
echo "=== pvrservice pid $P ==="
echo "--- pvr modules mapped ---"
grep -oE '/system/lib64/lib[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo
echo "=== any dlopen failure anywhere in the log ==="
logcat -d 2>/dev/null | grep -iE 'dlopen failed|cannot locate|CANNOT LINK' | tail -10
echo
echo "=== what decides whether the platform module loads QVR ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -iE 'qvr' | sort -u | head -20
echo
echo "=== the 6dof / tracker strings it uses ==="
strings -a /system/lib64/libpvrmodule_platform.so 2>/dev/null | grep -iE '6dof|tracker|svr' | sort -u | head -20

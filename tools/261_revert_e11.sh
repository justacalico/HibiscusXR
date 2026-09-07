#!/system/bin/sh
# I patched libPvr_UnitySDKExt11.so using a cave the generic script picked without
# checking whether it was really dead space. objdump labels it
# _ZN3PVR12BufferedFileD0Ev+0x44 - inside a function's range - so it may be live
# code I overwrote. Restore the original and retest before blaming anything else.
T=/system/lib64/libPvr_UnitySDKExt11.so
mount -o rw,remount /system
if [ ! -f "$T.orig" ]; then echo "NO BACKUP"; exit 1; fi
cp -a "$T.orig" "$T.restore"
mv -f "$T.restore" "$T"
sync
echo "restored:"
md5sum "$T" "$T.orig"
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 16
P=$(pidof com.pvr.vrshell)
echo "=== vrshell pid $P, threads $(ls /proc/$P/task 2>/dev/null | wc -l) ==="
logcat -d 2>/dev/null | grep -iE 'EnterVrMode|Instantiate TimeWarp|InitRenderThread|registerClient|SelectRT' | tail -8

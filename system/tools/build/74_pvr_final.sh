#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp /data/local/tmp/pvrservice.rc /system/etc/init/pvrservice.rc
chmod 644 /system/etc/init/pvrservice.rc
chown root:root /system/etc/init/pvrservice.rc
# make sure the shim is actually present on this boot
cp -f /data/local/tmp/libshim_pvr.so /system/lib64/libshim_pvr.so 2>/dev/null
chmod 644 /system/lib64/libshim_pvr.so
chown root:root /system/lib64/libshim_pvr.so
sync
mount -o ro,remount /system

echo "=== rc now ==="
grep -E 'service|seclabel|setenv' /system/etc/init/pvrservice.rc
echo "shim: $(ls -l /system/lib64/libshim_pvr.so)"
echo

# init parsed the OLD rc at boot, so setenv is not applied to the running one.
# Kill it and start with the preload by hand; the next reboot proves the init path.
echo "=== restart with the shim ==="
setprop sys.pvr.vrservice.state 0
sleep 1
pkill -f /system/bin/pvrservice 2>/dev/null
sleep 1
LD_PRELOAD=/system/lib64/libshim_pvr.so nohup /system/bin/pvrservice \
    >/data/local/tmp/pvr.final.out 2>&1 &
sleep 6
P1=$(pidof pvrservice)
echo "pid after 6s : $P1"
sleep 15
P2=$(pidof pvrservice)
echo "pid after 21s: $P2   $([ "$P1" = "$P2" ] && echo STABLE || echo RESTARTED)"
echo

echo "=== modules ==="
logcat -d 2>/dev/null | grep -E 'Load libpvrmodule|load .* module failed' | tail -6
echo
echo "=== live pose ==="
logcat -d 2>/dev/null | grep 'getTrackingDataExt rotation' | tail -4
echo
echo "=== any stack corruption? ==="
logcat -d -b crash 2>/dev/null | grep -c 'stack corruption'
echo DONE

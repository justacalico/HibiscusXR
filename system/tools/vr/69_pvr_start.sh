#!/system/bin/sh
# Fix ownership (cp -a preserved shell:shell from /data/local/tmp) and try to
# bring pvrservice up.
mount -o rw,remount /system
for d in /system/bin /system/lib /system/lib64 /system/framework /system/etc/pvr; do
  find $d -newer /system/build.prop -exec chown root:root {} \; 2>/dev/null
done
chown root:root /system/bin/pvrservice /system/bin/qvrservice /system/bin/pvr_compute /system/bin/vr
chown -R root:root /system/etc/pvr
chmod 755 /system/bin/pvrservice /system/bin/qvrservice /system/bin/pvr_compute /system/bin/vr
sync
mount -o ro,remount /system
echo "=== ownership now ==="
ls -l /system/bin/pvrservice /system/lib64/libcompositor.pxr.so
echo

echo "=== did init parse pvrservice.rc? ==="
dmesg | grep -i 'pvrservice' | head -10
echo

echo "=== try starting it ==="
setprop sys.pvr.vrservice.state 1
sleep 2
echo "init.svc.pvrservice = $(getprop init.svc.pvrservice)"
echo "pid = $(pidof pvrservice)"
echo

echo "=== if it did not start, run it by hand to see the real error ==="
if [ -z "$(pidof pvrservice)" ]; then
  /system/bin/pvrservice 2>&1 | head -20 &
  sleep 3
  echo "--- manual run pid: $(pidof pvrservice) ---"
fi
echo

echo "=== linker view: what does pvrservice need and can it find it? ==="
LD_DEBUG=1 /system/bin/pvrservice 2>&1 | head -5
echo "---"
# the real question on Android 10: can a /system binary reach /vendor libs?
echo "vendor client libs pvrservice may want:"
ls -l /vendor/lib64/libqvrservice_client.so /vendor/lib64/libpvr.so 2>&1
echo

echo "=== recent logcat for pvr/qvr ==="
logcat -d 2>/dev/null | grep -iE 'pvrservice|qvrservice|pxr|linker.*pvr' | tail -25
echo DONE

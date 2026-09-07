#!/system/bin/sh
exec 2>&1
pkill -f /system/bin/pvrservice 2>/dev/null
sleep 1

mount -o rw,remount /system
cp /data/local/tmp/libshim_pvr.so /system/lib64/libshim_pvr.so
chmod 644 /system/lib64/libshim_pvr.so
chown root:root /system/lib64/libshim_pvr.so
sync
mount -o ro,remount /system
ls -l /system/lib64/libshim_pvr.so
echo

echo "=== does the shim resolve libpvrmodule_platform.so now? ==="
# direct check: dlopen the module with the shim preloaded
cat > /data/local/tmp/dltest.sh <<'EOF'
LD_PRELOAD=/system/lib64/libshim_pvr.so /system/bin/pvrservice
EOF

echo "=== start pvrservice WITH the shim ==="
LD_PRELOAD=/system/lib64/libshim_pvr.so nohup /system/bin/pvrservice \
    >/data/local/tmp/pvrservice.shim.out 2>&1 &
sleep 5
echo "pid = $(pidof pvrservice)"
echo

echo "=== platform module: loaded or still failing? ==="
logcat -d 2>/dev/null | grep -iE 'PvrService.*(platform|Open library|module)' | tail -15
echo

echo "=== is libpvrmodule_platform actually mapped in? ==="
P=$(pidof pvrservice)
if [ -n "$P" ]; then
  grep -oE '/system/lib64/lib(pvrmodule|compositor|runtime|shim)[^ ]*\.so' /proc/$P/maps | sort -u
fi
echo

echo "=== tracking output ==="
logcat -d 2>/dev/null | grep -iE 'getTrackingDataExt|setShareMemoryData|PvrService' | tail -12
echo DONE

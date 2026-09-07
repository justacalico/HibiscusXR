#!/system/bin/sh
# Install the seclabel'd pvrservice.rc and bring the service up under init.
# Output is fully redirected and nothing is left attached to this shell - a
# backgrounded daemon holding stdout is what wedged the previous run.
exec 2>&1

pkill -f /system/bin/pvrservice 2>/dev/null
sleep 1

mount -o rw,remount /system
cp /data/local/tmp/pvrservice.rc /system/etc/init/pvrservice.rc
chmod 644 /system/etc/init/pvrservice.rc
chown root:root /system/etc/init/pvrservice.rc
sync
mount -o ro,remount /system
echo "rc installed:"
cat /system/etc/init/pvrservice.rc | head -8
echo

# init only parses .rc files at boot, so this run still needs a manual start.
# The reboot after this is what proves the init path works.
echo "=== manual start (detached) ==="
nohup /system/bin/pvrservice >/data/local/tmp/pvrservice.out 2>&1 &
sleep 4
echo "pid = $(pidof pvrservice)"
echo
echo "=== its own output ==="
head -30 /data/local/tmp/pvrservice.out
echo
echo "=== logcat ==="
logcat -d 2>/dev/null | grep -iE 'pvr|pxr|qvr' | tail -30
echo
echo "=== what did it open / connect to? ==="
P=$(pidof pvrservice)
if [ -n "$P" ]; then
  echo "--- libs loaded ---"
  grep -oE '/(system|vendor)/[^ ]*\.so' /proc/$P/maps | sort -u | head -30
  echo "--- sockets / devices ---"
  ls -l /proc/$P/fd 2>/dev/null | grep -oE '(socket|/dev/[^ ]*)' | sort | uniq -c | head -20
fi
echo DONE

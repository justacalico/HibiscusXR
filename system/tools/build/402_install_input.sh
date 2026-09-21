#!/system/bin/sh
# Install the patched libinput (adds DEFINE_CONFIRM/HALL_OPEN/HALL_CLOSE/DC_IN)
# and put the full gpio-keys.kl back, including the DEFINE_CONFIRM line I removed
# while testing.
set -u
mount -o rw,remount /system

echo "=== back up the originals once ==="
[ -f /data/local/tmp/libinput64.orig.so ] || cp /system/lib64/libinput.so /data/local/tmp/libinput64.orig.so
[ -f /data/local/tmp/libinput32.orig.so ] || cp /system/lib/libinput.so   /data/local/tmp/libinput32.orig.so
ls -l /data/local/tmp/libinput*.orig.so

echo
echo "=== install (temp + mv: never write over a mapped .so) ==="
cp /data/local/tmp/ourinput64.patched.so /system/lib64/libinput.so.new
cp /data/local/tmp/ourinput32.patched.so /system/lib/libinput.so.new
for f in /system/lib64/libinput.so /system/lib/libinput.so; do
    chmod 644 "$f.new"; chown root:root "$f.new"
    chcon u:object_r:system_lib_file:s0 "$f.new" 2>/dev/null
    mv -f "$f.new" "$f"
done
sync
ls -l /system/lib64/libinput.so /system/lib/libinput.so

echo
echo "=== restore the full key layout ==="
cp -f /data/local/tmp/gpio-keys.kl /system/usr/keylayout/gpio-keys.kl
chmod 644 /system/usr/keylayout/gpio-keys.kl
chown root:root /system/usr/keylayout/gpio-keys.kl
chcon u:object_r:system_file:s0 /system/usr/keylayout/gpio-keys.kl 2>/dev/null
sync
tail -8 /system/usr/keylayout/gpio-keys.kl

echo
echo "=== rebooting so the input reader re-enumerates ==="
setprop persist.adb.tcp.port 5555
sync

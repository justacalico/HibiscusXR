#!/system/bin/sh
# Install Pico's key layouts. Without gpio-keys.kl the headset buttons fall
# through Generic.kl, so scancode 28 (Confirm) arrives as ENTER and scancode 158
# is not BACK - which is why gaze + Confirm does not work.
#
# DEFINE_CONFIRM is a Pico-specific keycode label. Their framework defines it;
# LineageOS's may not, in which case the parser will complain about that line.
set -u
mount -o rw,remount /system

cp -f /data/local/tmp/gpio-keys.kl /system/usr/keylayout/gpio-keys.kl
cp -f /data/local/tmp/dc_detect.kl /system/usr/keylayout/dc_detect.kl
chmod 644 /system/usr/keylayout/gpio-keys.kl /system/usr/keylayout/dc_detect.kl
chown root:root /system/usr/keylayout/gpio-keys.kl /system/usr/keylayout/dc_detect.kl
chcon u:object_r:system_file:s0 /system/usr/keylayout/gpio-keys.kl /system/usr/keylayout/dc_detect.kl 2>/dev/null
sync
ls -l /system/usr/keylayout/gpio-keys.kl /system/usr/keylayout/dc_detect.kl

echo
echo "=== make the framework re-read them ==="
logcat -c
# input reader picks up layout changes when the device is re-enumerated; the
# cheap way is to bounce the input flinger via a config change
am force-stop com.pvr.vrshell 2>/dev/null
sleep 2
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 12

echo
echo "=== did the parser accept DEFINE_CONFIRM? ==="
logcat -d | grep -iE "keylayout|KeyLayoutMap|Invalid keycode|gpio-keys" | tail -12
echo "  (empty = no complaints)"

echo
echo "=== which layout is gpio-keys using now? ==="
dumpsys input 2>/dev/null | grep -A3 "gpio-keys" | head -8

#!/system/bin/sh
# A single bad line makes KeyLayoutMap::load fail, and EventHub then falls back to
# Generic.kl for the whole device. DEFINE_CONFIRM is a Pico-specific label their
# framework defines and LineageOS's does not.
echo "=== parse errors across all log buffers ==="
logcat -d -b main -b system -b crash 2>/dev/null | grep -iE "keylayout|KeyLayoutMap|KeyCharacterMap|EventHub|Invalid keycode|unknown keycode|Error .* parsing" | head -20
echo "  (nothing = rotated out; force a reload below)"

echo
echo "=== force a reload with the suspect line removed, and watch ==="
cp /system/usr/keylayout/gpio-keys.kl /data/local/tmp/gpio-keys.kl.orig
mount -o rw,remount /system
grep -v DEFINE_CONFIRM /system/usr/keylayout/gpio-keys.kl > /data/local/tmp/gk.test
cp -f /data/local/tmp/gk.test /system/usr/keylayout/gpio-keys.kl
chmod 644 /system/usr/keylayout/gpio-keys.kl
chcon u:object_r:system_file:s0 /system/usr/keylayout/gpio-keys.kl 2>/dev/null
sync
echo "  removed the DEFINE_CONFIRM line; tail is now:"
tail -6 /system/usr/keylayout/gpio-keys.kl

echo
echo "=== does the framework accept it now? (needs re-enumeration) ==="
logcat -c
# toggling the device off/on through the driver is not available; restart the
# system server's input by bouncing the shell is not enough - report and let the
# caller reboot
echo "  installed. A reboot is required for the input reader to re-read it."

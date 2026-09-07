#!/system/bin/sh
echo "=== is the file still there after the reboot? ==="
ls -lZ /system/usr/keylayout/gpio-keys.kl /system/usr/keylayout/dc_detect.kl 2>/dev/null || echo "  GONE"

echo
echo "=== compare with a layout that IS being used ==="
ls -lZ /system/usr/keylayout/Generic.kl

echo
echo "=== byte-compare against stock's copy ==="
md5sum /system/usr/keylayout/gpio-keys.kl 2>/dev/null
echo "  stock gpio-keys.kl should be 1892 bytes"

echo
echo "=== does anything else provide a gpio-keys layout that wins? ==="
for d in /odm/usr/keylayout /vendor/usr/keylayout /product/usr/keylayout /data/system/devices/keylayout; do
  [ -d "$d" ] && ls -l "$d" 2>/dev/null | head -5
done

echo
echo "=== exact device name the reader sees ==="
cat /sys/class/input/event2/device/name 2>/dev/null
getevent -pl /dev/input/event2 2>/dev/null | head -6

echo
echo "=== CRLF? a stray carriage return breaks the parse silently ==="
head -c 200 /system/usr/keylayout/gpio-keys.kl | od -c 2>/dev/null | head -4
echo "  --- tail, where the key lines are ---"
tail -12 /system/usr/keylayout/gpio-keys.kl | od -c 2>/dev/null | tail -12

#!/system/bin/sh
# Headset buttons: with no controllers, gaze + Confirm is the only interaction,
# and Confirm is not mapped. Compare the input plumbing against stock.
echo "=== input devices the kernel exposes ==="
for d in /dev/input/event*; do
  n=$(cat /sys/class/input/$(basename $d)/device/name 2>/dev/null)
  echo "  $d  $n"
done

echo
echo "=== what the framework thinks it has ==="
dumpsys input 2>/dev/null | grep -iE "^ *[0-9]+: |Descriptor|KeyLayoutFile|Name:|Sources" | head -40

echo
echo "=== key layout files present ==="
ls /system/usr/keylayout/ 2>/dev/null | grep -iE "pvr|pico|vr|gpio|qpnp|Generic"
echo "  --- all ---"
ls /system/usr/keylayout/ 2>/dev/null | wc -l
ls /vendor/usr/keylayout/ 2>/dev/null

echo
echo "=== pico-specific layouts and their contents ==="
for f in /system/usr/keylayout/*pvr* /system/usr/keylayout/*pico* /vendor/usr/keylayout/*; do
  [ -f "$f" ] || continue
  echo "  --- $f ---"
  cat "$f" 2>/dev/null | head -20
done

echo
echo "=== key character maps ==="
ls /system/usr/keychars/ 2>/dev/null | grep -iE "pvr|pico|vr"

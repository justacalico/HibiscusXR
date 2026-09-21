#!/system/bin/sh
# Pico's headset Back/Confirm are not Android BACK. Find the real key layout and
# which input device produces them.
echo "=== key layout files mentioning pico/hmd/headset ==="
ls /system/usr/keylayout/ 2>/dev/null
echo
for f in /system/usr/keylayout/*.kl; do
  case "$f" in
    *gpio*|*pico*|*pvr*|*hmd*|*headset*|*Generic*)
      echo "--- $f ---"; cat "$f" ;;
  esac
done 2>/dev/null | head -60
echo
echo "=== input devices ==="
getevent -pl 2>/dev/null | grep -iE 'name:|KEY_' | head -40
echo
echo "=== display ids (vr may be on a virtual display) ==="
dumpsys display 2>/dev/null | grep -iE 'mDisplayId|uniqueId|displayName' | head -12

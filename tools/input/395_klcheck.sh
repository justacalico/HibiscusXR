#!/system/bin/sh
echo "=== KeyLayoutFile per device ==="
dumpsys input 2>/dev/null | grep -E "^ +[0-9]+: |KeyLayoutFile" | head -30

echo
echo "=== does the framework know DEFINE_CONFIRM at all? ==="
# if the label is unknown the parser logs and skips; force a parse by asking
# for the device's key mapping
dumpsys input 2>/dev/null | grep -iE "DEFINE_CONFIRM|Invalid|Unknown key" | head

echo
echo "=== live scancodes: press a headset button in the next 12 s ==="
timeout 12 getevent -lq /dev/input/event2 2>/dev/null | head -20
echo "  (nothing above = no button pressed during the window)"

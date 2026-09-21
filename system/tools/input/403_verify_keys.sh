#!/system/bin/sh
echo "=== uptime ==="
awk '{printf "  up %d s\n", $1}' /proc/uptime

echo
echo "=== did gpio-keys finally take Pico's layout? ==="
dumpsys input 2>/dev/null | grep -B2 -A8 "gpio-keys" | grep -E "gpio-keys|KeyLayoutFile" | head -4
echo
echo "=== and dc_detect ==="
dumpsys input 2>/dev/null | grep -A8 "dc_detect" | grep -E "dc_detect|KeyLayoutFile" | head -4

echo
echo "=== any keylayout parse errors left ==="
logcat -d -b main -b system 2>/dev/null | grep -iE "keylayout|KeyLayoutMap|Invalid keycode" | head -6
echo "  (empty = parsed cleanly)"

echo
echo "=== system health after patching libinput ==="
echo "  system_server [$(pidof system_server)]"
echo "  vrshell       [$(pidof com.pvr.vrshell)]"
echo "  pvrservice    [$(pidof pvrservice)]"
echo "  fancontrol    [$(pidof fancontrol)]  rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== press the headset Confirm button now (12 s window) ==="
echo "  watching /dev/input/event2 for scancodes:"
timeout 12 getevent -lq /dev/input/event2 2>/dev/null | head -12
echo "  --- and what the framework made of it ---"
logcat -d 2>/dev/null | grep -iE "KeyEvent|keycode=1001|DEFINE_CONFIRM|dispatchKey" | tail -8

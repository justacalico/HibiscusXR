#!/system/bin/sh
echo "=== uptime ==="
awk '{printf "  up %d s\n", $1}' /proc/uptime

echo
echo "=== did gpio-keys pick up Pico's layout? ==="
dumpsys input 2>/dev/null | grep -B1 -A6 "gpio-keys" | grep -E "gpio-keys|KeyLayoutFile" | head -6

echo
echo "=== any keylayout parse complaints (DEFINE_CONFIRM is a Pico label) ==="
logcat -d | grep -iE "keylayout|KeyLayoutMap|Invalid keycode|unknown key" | head -8
echo "  (empty = parsed cleanly)"

echo
echo "=== did init start the fan daemon from the rc this time? ==="
echo "  init.svc.pn2_fancontrol = [$(getprop init.svc.pn2_fancontrol)]"
echo "  fancontrol pid [$(pidof fancontrol)]"
echo "  fan rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null) state=$(cat /sys/class/thermal/cooling_device1/cur_state 2>/dev/null)"

echo
echo "=== stack health after a clean boot ==="
echo "  vrshell    [$(pidof com.pvr.vrshell)]"
echo "  pvrservice [$(pidof pvrservice)]"
echo "  airservice [$(pidof airservice)]"
echo "  segv=$(logcat -d | grep -c 'exited due to signal 11')"
echo "  kLost=$(logcat -d | grep -c kLostDialog)  BadPose=$(logcat -d | grep -c 'Bad Pose')"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

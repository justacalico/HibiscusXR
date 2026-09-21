#!/system/bin/sh
# Put the unit back the way it was before the gate tests.
set -u
echo "=== restarting the pico layer ==="
start pvrservice 2>/dev/null
start airservice 2>/dev/null
sleep 4

echo "=== relaunching the home shell ==="
# .MainActivity was wrong, ask the package manager what the launcher entry is
cmp=$(cmd package resolve-activity --brief -c android.intent.category.LAUNCHER com.pvr.vrshell 2>/dev/null | tail -1)
echo "  launcher component: ${cmp:-<none>}"
[ -n "$cmp" ] && am start -n "$cmp" > /dev/null 2>&1
sleep 8

echo
echo "=== state ==="
for p in pvrservice airservice qvrservice fancontrol adsprpcd cdsprpcd; do
  echo "  $p = $(pidof $p 2>/dev/null || echo DEAD)"
done
echo "  vrshell = $(pidof com.pvr.vrshell 2>/dev/null || echo DEAD)"
echo "  fan rpm = $(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"
dumpsys window 2>/dev/null | grep -m1 mCurrentFocus

echo
echo "=== is tracking actually alive right now? ==="
logcat -d 2>/dev/null | grep -iE "pose_quality|BadPose|kLost|tracking_state|6dof" | tail -8

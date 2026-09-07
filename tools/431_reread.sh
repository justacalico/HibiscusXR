#!/system/bin/sh
# cat -n is not a thing on toybox. Re-read the log I already captured.
L=/data/local/tmp/lit6.log
echo "########## quality lines, verbatim ##########"
grep "pose_quality" $L | head -12
echo "  ..."
grep "pose_quality" $L | tail -6

echo
echo "########## unique quality tuples ##########"
grep -o "pose_quality = [0-9.]*" $L | sort | uniq -c
grep -o "sensor_quality=[0-9.]*" $L | sort | uniq -c
grep -o "camera_quality=[0-9.]*" $L | sort | uniq -c
grep -o "tracking_warning_flags=[0-9]*" $L | sort | uniq -c

echo
echo "########## full first pose block ##########"
grep -B1 -A3 "Head tracking pose" $L | head -12

echo
echo "########## fan: it read 0 at the end of the test ##########"
echo "  fancontrol pid = $(pidof fancontrol 2>/dev/null || echo DEAD)"
echo "  init.svc       = $(getprop init.svc.pn2_fancontrol)"
echo "  fan1_input     = $(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"
echo "  cur_state      = $(cat /sys/class/thermal/cooling_device0/cur_state 2>/dev/null)"
echo "  thermal zones (top 5):"
for z in /sys/class/thermal/thermal_zone*/temp; do
  t=$(cat $z 2>/dev/null); n=$(cat $(dirname $z)/type 2>/dev/null)
  [ -n "$t" ] && echo "$t $n"
done | sort -rn | head -5

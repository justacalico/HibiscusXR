#!/system/bin/sh
# Device is heating with no fan. Shed the load first, then find the fan control.
echo "=== shed load ==="
am force-stop com.pvr.vrshell
am force-stop com.pvr.seethrough.setting
pm disable com.picovr.picovrlib.cvcontroller >/dev/null 2>&1
am force-stop com.picovr.picovrlib.cvcontroller
stop airservice 2>/dev/null
stop pvrservice 2>/dev/null
sleep 2
echo "  vr stack stopped"
echo
echo "=== temperatures now ==="
for z in /sys/class/thermal/thermal_zone*; do
  t=$(cat $z/temp 2>/dev/null); n=$(cat $z/type 2>/dev/null)
  [ -n "$t" ] && [ "$t" -gt 45000 ] 2>/dev/null && echo "  $n = $((t/1000)) C"
done
echo "  (only zones above 45 C listed)"
echo
echo "=== fan / cooling devices ==="
for c in /sys/class/thermal/cooling_device*; do
  n=$(cat $c/type 2>/dev/null)
  case "$n" in *fan*|*FAN*|*Fan*) echo "  $c type=$n cur=$(cat $c/cur_state 2>/dev/null) max=$(cat $c/max_state 2>/dev/null)";; esac
done
ls -d /sys/class/hwmon/* 2>/dev/null | while read h; do
  echo "  $h name=$(cat $h/name 2>/dev/null)"
  ls $h 2>/dev/null | grep -iE "pwm|fan" | sed 's/^/    /'
done
echo
echo "=== any fan node anywhere ==="
find /sys -maxdepth 6 -iname "*fan*" 2>/dev/null | head -20
echo
echo "=== who controls it on this build ==="
ls -l /vendor/bin/*fan* /system/bin/*fan* /vendor/bin/hw/*thermal* 2>/dev/null
getprop | grep -iE "fan|thermal" | head -10

#!/system/bin/sh
# Reference the stock headset: what actually drives the fan there, and what does
# the factory app touch? Our gpio-fan node accepts writes and reports an rpm, but
# the fan does not physically spin - so the real control is somewhere else.
echo "=== gpio-fan node state ==="
for c in /sys/class/thermal/cooling_device*; do
  t=$(cat $c/type 2>/dev/null)
  case "$t" in *fan*) echo "  $c type=$t cur=$(cat $c/cur_state 2>/dev/null) max=$(cat $c/max_state 2>/dev/null)";; esac
done
H=/sys/class/hwmon/hwmon1
[ "$(cat $H/name 2>/dev/null)" = "gpio_fan" ] || for h in /sys/class/hwmon/*; do [ "$(cat $h/name 2>/dev/null)" = "gpio_fan" ] && H=$h; done
echo "  hwmon=$H"
for f in pwm1 pwm1_enable pwm1_mode pwm_rpm fan1_input fan1_target fan1_min fan1_max; do
  [ -e "$H/$f" ] && echo "    $f = $(cat $H/$f 2>/dev/null)"
done
echo
echo "=== devicetree gpio-fan ==="
DT=/sys/firmware/devicetree/base/soc/gpio_fan
ls $DT 2>/dev/null | sed 's/^/    /'
echo "  speed-map:"; xxd $DT/gpio-fan,speed-map 2>/dev/null | head -4
echo "  status:"; cat $DT/status 2>/dev/null; echo
echo "  compatible:"; cat $DT/compatible 2>/dev/null; echo
echo
echo "=== factory / fan-related apps ==="
pm list packages 2>/dev/null | grep -iE "factory|fan|test|hardware|tool" | head -12
echo
echo "=== anything referencing the fan node ==="
for p in /system/bin /system/priv-app /vendor/bin; do
  grep -rl "gpio_fan\|cooling_device1\|fan1_input\|pwm_rpm" $p 2>/dev/null | head -6
done
echo
echo "=== fan-related props ==="
getprop | grep -iE "fan|rpm" | head -10
echo
echo "=== running services that could own it ==="
ps -A 2>/dev/null | grep -iE "thermal|fan|factory" | head -8

echo "=== before ==="
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state 2>/dev/null) max=$(cat /sys/class/thermal/cooling_device1/max_state 2>/dev/null)"
echo "  pwm1=$(cat /sys/class/hwmon/hwmon1/pwm1 2>/dev/null) rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"
echo
echo "=== turn the fan on (max) ==="
echo 3 > /sys/class/thermal/cooling_device1/cur_state 2>/dev/null || echo "  cur_state write FAILED"
echo 255 > /sys/class/hwmon/hwmon1/pwm1 2>/dev/null
sleep 4
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state 2>/dev/null)"
echo "  pwm1=$(cat /sys/class/hwmon/hwmon1/pwm1 2>/dev/null) rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null) pwm_rpm=$(cat /sys/class/hwmon/hwmon1/pwm_rpm 2>/dev/null)"
echo
echo "=== hottest zones after 15s ==="
sleep 15
for z in /sys/class/thermal/thermal_zone*; do
  t=$(cat $z/temp 2>/dev/null); n=$(cat $z/type 2>/dev/null)
  [ -n "$t" ] && [ "$t" -gt 70000 ] 2>/dev/null && echo "  $n = $((t/1000)) C"
done | head -8
echo
echo "=== fan state again ==="
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state) rpm=$(cat /sys/class/hwmon/hwmon1/fan1_input)"

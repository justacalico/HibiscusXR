echo "--- how many instances? ---"
pidof fancontrol
pkill -f "^/system/bin/fancontrol" 2>/dev/null
sleep 2
setprop persist.pxr.log.fancontrol 1
logcat -c
nohup /system/bin/fancontrol >/dev/null 2>&1 &
sleep 6
echo "  single pid now: $(pidof fancontrol)"
echo
echo "--- idle behaviour over 60s (temps are falling, fan should step down) ---"
H=/sys/class/hwmon/hwmon1
i=0
while [ $i -lt 6 ]; do
  hot=0
  for z in /sys/class/thermal/thermal_zone*; do
    ty=$(cat $z/type 2>/dev/null)
    case "$ty" in cpu*-usr|gpu*-usr) v=$(cat $z/temp 2>/dev/null); [ -n "$v" ] && [ "$v" -gt "$hot" ] 2>/dev/null && hot=$v;; esac
  done
  echo "  t+$((i*10))s  hottest=$((hot/1000))C  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)  pwm1=$(cat $H/pwm1)  rpm=$(cat $H/fan1_input)"
  i=$((i+1)); sleep 10
done
echo
echo "--- what fancontrol decided ---"
logcat -d | grep -iE "fancontrol|FanControl" | tail -15
echo "  (empty = it logs only above a threshold)"

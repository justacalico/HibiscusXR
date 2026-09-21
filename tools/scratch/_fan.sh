mount -o rw,remount /system
cp -f /data/local/tmp/fancontrol /system/bin/fancontrol
chmod 755 /system/bin/fancontrol
chown root:shell /system/bin/fancontrol
chcon u:object_r:system_file:s0 /system/bin/fancontrol 2>/dev/null
cp -f /data/local/tmp/pn2-fanservice.rc /system/etc/init/pn2-fanservice.rc
chmod 644 /system/etc/init/pn2-fanservice.rc
chown root:root /system/etc/init/pn2-fanservice.rc
chcon u:object_r:system_file:s0 /system/etc/init/pn2-fanservice.rc 2>/dev/null
mkdir -p /data/picovr/fan 2>/dev/null
chmod 755 /data/picovr /data/picovr/fan 2>/dev/null
sync
ls -l /system/bin/fancontrol /system/etc/init/pn2-fanservice.rc
echo
echo "--- fan before ---"
H=/sys/class/hwmon/hwmon1
echo "  pwm1=$(cat $H/pwm1) pwm1_enable=$(cat $H/pwm1_enable) rpm=$(cat $H/fan1_input)"
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)"
echo
echo "--- run it directly first (init only parses the new rc on boot) ---"
# detach fully: it never exits, and a child holding stdout keeps adb shell open
nohup /system/bin/fancontrol >/dev/null 2>&1 &
sleep 12
echo "  fancontrol pid $(pidof fancontrol)"
echo "--- fan after ---"
echo "  pwm1=$(cat $H/pwm1) pwm1_enable=$(cat $H/pwm1_enable) rpm=$(cat $H/fan1_input)"
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)"
echo "--- what it logged ---"
logcat -d | grep -iE "fancontrol|FanControl" | tail -12
echo "--- hottest zones ---"
for z in /sys/class/thermal/thermal_zone*; do t=$(cat $z/temp 2>/dev/null); n=$(cat $z/type 2>/dev/null); [ -n "$t" ] && [ "$t" -gt 60000 ] 2>/dev/null && echo "  $n = $((t/1000)) C"; done | head -6

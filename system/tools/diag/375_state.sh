#!/system/bin/sh
echo "=== uptime (did it actually reboot?) ==="
awk '{printf "  up %d min %d s\n", $1/60, $1%60}' /proc/uptime
echo
echo "=== fan ==="
echo "  init.svc.pn2_fancontrol = [$(getprop init.svc.pn2_fancontrol)]"
echo "  fancontrol pid: $(pidof fancontrol)"
H=/sys/class/hwmon/hwmon1
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state) pwm1=$(cat $H/pwm1) rpm=$(cat $H/fan1_input)"
echo
echo "=== adb-over-wifi persistence ==="
echo "  persist.adb.tcp.port = [$(getprop persist.adb.tcp.port)]"
echo "  persist.pn2.adbwifi  = [$(getprop persist.pn2.adbwifi)]"
echo
echo "=== controller service ==="
pm list packages -e 2>/dev/null | grep -c cvcontroller
echo
echo "=== patched lib ==="
md5sum /system/lib/libpvrserviceclient.so
echo "  patched should be ad7b078f28b56049fc8dd72d5b4eed1a"
echo
echo "=== stale (deleted) mappings ==="
for p in $(pidof com.pvr.vrshell) $(pidof com.picovr.picovrlib.cvcontroller:RemoteService); do
  n=$(grep -c 'libpvrserviceclient.so (deleted)' /proc/$p/maps 2>/dev/null)
  echo "  pid $p -> $n"
done

echo "=== did init start the fan daemon? ==="
echo "  init.svc.pn2_fancontrol = $(getprop init.svc.pn2_fancontrol)"
echo "  pid $(pidof fancontrol)"
H=/sys/class/hwmon/hwmon1
echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state) pwm1=$(cat $H/pwm1) rpm=$(cat $H/fan1_input)"
echo
echo "=== controller service ==="
echo "  enabled=$(pm list packages -e | grep -c cvcontroller)"
echo
echo "=== patched lib, and is anything on a stale inode? ==="
md5sum /system/lib/libpvrserviceclient.so
echo "  (patched = ad7b078f28b56049fc8dd72d5b4eed1a)"
echo
echo "=== launch the shell ==="
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 30
V=$(pidof com.pvr.vrshell)
echo "  vrshell pid $V threads=$(ls /proc/$V/task 2>/dev/null | wc -l)"
echo "  segv=$(logcat -d | grep -c \"exited due to signal 11\")"
echo "  deleted-maps: $(grep -c \"libpvrserviceclient.so (deleted)\" /proc/$V/maps 2>/dev/null)"
C=$(pidof com.picovr.picovrlib.cvcontroller:RemoteService)
echo "  cvservice pid ${C:-none}  deleted-maps: $(grep -c \"libpvrserviceclient.so (deleted)\" /proc/${C%% *}/maps 2>/dev/null)"
echo
echo "=== tracking / compositor ==="
echo "  BadPose=$(logcat -d | grep -c \"Bad Pose\")  kLost=$(logcat -d | grep -c kLostDialog)"
logcat -d | grep -iE "getTrackingDataExt position" | tail -2
echo
echo "=== pvrservice health ==="
P=$(pidof pvrservice)
echo "  contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)"
echo
echo "=== temps ==="
for z in /sys/class/thermal/thermal_zone*; do t=$(cat $z/temp 2>/dev/null); n=$(cat $z/type 2>/dev/null); case "$n" in cpu0-silver-usr|cpu3-gold-usr|gpu0-usr) echo "  $n = $((t/1000)) C";; esac; done

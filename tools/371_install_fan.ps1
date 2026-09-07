# Install Pico's fancontrol daemon.
#
# Writing hwmon1/pwm1 by hand did not physically spin the fan even though the node
# reported 11400 rpm, so the node alone is not the whole story - the vendor daemon
# is what actually drives it. It is a small self-contained binary (thermal_zone
# reads -> pwm1_enable/pwm1 writes) and every library it needs is already present.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\371_fan.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== fancontrol install $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

& $adb -s $Dev push F:\PN2Lineage\fan\fancontrol /data/local/tmp/fancontrol 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\overlay\etc\init\pn2-fanservice.rc /data/local/tmp/ 2>&1 | Out-Null

$sh = @(
  'mount -o rw,remount /system',
  'cp -f /data/local/tmp/fancontrol /system/bin/fancontrol',
  'chmod 755 /system/bin/fancontrol',
  'chown root:shell /system/bin/fancontrol',
  'chcon u:object_r:system_file:s0 /system/bin/fancontrol 2>/dev/null',
  'cp -f /data/local/tmp/pn2-fanservice.rc /system/etc/init/pn2-fanservice.rc',
  'chmod 644 /system/etc/init/pn2-fanservice.rc',
  'chown root:root /system/etc/init/pn2-fanservice.rc',
  'chcon u:object_r:system_file:s0 /system/etc/init/pn2-fanservice.rc 2>/dev/null',
  'mkdir -p /data/picovr/fan 2>/dev/null',
  'chmod 755 /data/picovr /data/picovr/fan 2>/dev/null',
  'sync',
  'ls -l /system/bin/fancontrol /system/etc/init/pn2-fanservice.rc',
  'echo',
  'echo "--- fan before ---"',
  'H=/sys/class/hwmon/hwmon1',
  'echo "  pwm1=$(cat $H/pwm1) pwm1_enable=$(cat $H/pwm1_enable) rpm=$(cat $H/fan1_input)"',
  'echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)"',
  'echo',
  'echo "--- run it directly first (init only parses the new rc on boot) ---"',
  '# detach fully: it never exits, and a child holding stdout keeps adb shell open',
  'nohup /system/bin/fancontrol >/dev/null 2>&1 &',
  'sleep 12',
  'echo "  fancontrol pid $(pidof fancontrol)"',
  'echo "--- fan after ---"',
  'echo "  pwm1=$(cat $H/pwm1) pwm1_enable=$(cat $H/pwm1_enable) rpm=$(cat $H/fan1_input)"',
  'echo "  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)"',
  'echo "--- what it logged ---"',
  'logcat -d | grep -iE "fancontrol|FanControl" | tail -12',
  'echo "--- hottest zones ---"',
  'for z in /sys/class/thermal/thermal_zone*; do t=$(cat $z/temp 2>/dev/null); n=$(cat $z/type 2>/dev/null); [ -n "$t" ] && [ "$t" -gt 60000 ] 2>/dev/null && echo "  $n = $((t/1000)) C"; done | head -6'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_fan.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_fan.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_fan.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

# Confirm fancontrol is a single instance and that it actually adapts.
# It references persist.pxr.log.fancontrol, so turn its logging on to see the
# decisions rather than inferring them from the node.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\372_fanverify.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== fancontrol verify $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- how many instances? ---"',
  'pidof fancontrol',
  'pkill -f "^/system/bin/fancontrol" 2>/dev/null',
  'sleep 2',
  'setprop persist.pxr.log.fancontrol 1',
  'logcat -c',
  'nohup /system/bin/fancontrol >/dev/null 2>&1 &',
  'sleep 6',
  'echo "  single pid now: $(pidof fancontrol)"',
  'echo',
  'echo "--- idle behaviour over 60s (temps are falling, fan should step down) ---"',
  'H=/sys/class/hwmon/hwmon1',
  'i=0',
  'while [ $i -lt 6 ]; do',
  '  hot=0',
  '  for z in /sys/class/thermal/thermal_zone*; do',
  '    ty=$(cat $z/type 2>/dev/null)',
  '    case "$ty" in cpu*-usr|gpu*-usr) v=$(cat $z/temp 2>/dev/null); [ -n "$v" ] && [ "$v" -gt "$hot" ] 2>/dev/null && hot=$v;; esac',
  '  done',
  '  echo "  t+$((i*10))s  hottest=$((hot/1000))C  cur_state=$(cat /sys/class/thermal/cooling_device1/cur_state)  pwm1=$(cat $H/pwm1)  rpm=$(cat $H/fan1_input)"',
  '  i=$((i+1)); sleep 10',
  'done',
  'echo',
  'echo "--- what fancontrol decided ---"',
  'logcat -d | grep -iE "fancontrol|FanControl" | tail -15',
  'echo "  (empty = it logs only above a threshold)"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_fv.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_fv.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_fv.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

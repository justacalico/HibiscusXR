# Make adbd listen on TCP across reboots so the headset no longer needs a physical
# replug after every boot. persist.adb.tcp.port is read by adbd at start, so baking
# it into build.prop makes it survive reboots (and it will land in the image too).
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\208_tcp.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== persistent tcp adb $(Get-Date) ==="

$sh = @(
  'mount -o rw,remount /system',
  '# drop any earlier attempt, then append with a guaranteed leading newline',
  'sed -i -E "/^persist\.adb\.tcp\.port=/d" /system/build.prop',
  '[ -n "$(tail -c1 /system/build.prop)" ] && echo "" >> /system/build.prop',
  'echo "persist.adb.tcp.port=5555" >> /system/build.prop',
  'sync',
  'setprop persist.adb.tcp.port 5555',
  'setprop service.adb.tcp.port 5555',
  'stop adbd; start adbd',
  'echo "--- build.prop tail ---"',
  'tail -3 /system/build.prop',
  'echo "--- sanity: no glued lines ---"',
  'grep -nE "^[a-z].*=.*[a-z]+\.[a-z].*=" /system/build.prop || echo "  clean"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_tcp.sh', $sh + "`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_tcp.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_tcp.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "--- device IP ---"
$ip = (& $adb -s $DEV shell "ip -f inet addr show wlan0 2>/dev/null | grep inet" 2>&1)
$ip | ForEach-Object { L ("   " + "$_".Trim()) }

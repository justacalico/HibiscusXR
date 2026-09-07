# Install the wireless-adb init trigger and turn it on for this dev unit.
param([string]$Dev = 'PA7B40NGE5300009W')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\211_adbwifi.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== wireless adb init trigger $(Get-Date) ==="

& $adb -s $Dev push F:\PN2Lineage\overlay\etc\init\pn2-adbwifi.rc /data/local/tmp/pn2-adbwifi.rc 2>&1 | Out-Null

$sh = @(
  'mount -o rw,remount /system',
  'cp /data/local/tmp/pn2-adbwifi.rc /system/etc/init/pn2-adbwifi.rc',
  'chmod 644 /system/etc/init/pn2-adbwifi.rc',
  'chown root:root /system/etc/init/pn2-adbwifi.rc',
  'sync',
  'ls -l /system/etc/init/pn2-adbwifi.rc',
  'echo "--- enabling ---"',
  'setprop persist.pn2.adbwifi 1',
  'sleep 3',
  'echo "service.adb.tcp.port = $(getprop service.adb.tcp.port)"',
  'echo "persist.pn2.adbwifi  = $(getprop persist.pn2.adbwifi)"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_adbwifi.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_adbwifi.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_adbwifi.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "--- connecting wirelessly ---"
Start-Sleep -Seconds 2
L ((& $adb connect 192.168.0.172:5555 2>&1) -join ' ')
(& $adb devices 2>&1 | Select-Object -Skip 1 | Where-Object { $_ -match '\S' }) | ForEach-Object { L ("   " + $_) }

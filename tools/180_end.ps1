# How does VRShell end now that it reaches Unity? Did it crash, or exit cleanly?
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'

Write-Host "=== tail of the vrshell process life ==="
(& $adb -s $DEV shell "logcat -d -b main,crash -v brief" 2>&1) |
  Where-Object { $_ -match 'Fatal signal|has died|exited due|Force finishing|UnityPlugin|EnterVrMode|Render thread|ANR|binderdied|vrshell' } |
  Select-Object -Last 35 | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== newest tombstone ==="
$n = ((& $adb -s $DEV shell "su -c 'ls -t /data/tombstones/tombstone_*'" 2>&1) | ForEach-Object { "$_".Trim() } | Select-Object -First 1)
Write-Host ("  file: " + $n)
if ($n) {
  (& $adb -s $DEV shell "su -c 'cat $n'" 2>&1) | Select-Object -First 26 | ForEach-Object { Write-Host ("  " + $_) }
}

# Reboot and watch for boot_completed. Writes a live log so progress is visible
# with:  Get-Content F:\PN2Lineage\notes\38_boot.log -Wait
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\38_boot.log'
function L($m) { $s = ("[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m); Add-Content $log $s; Write-Host $s }

Set-Content $log "=== boot watch after boot-HAL manifest fix $(Get-Date) ==="
L "rebooting"
& $adb reboot | Out-Null
Start-Sleep -Seconds 5

# wait for adb
for ($i=0; $i -lt 150; $i++) {
  $s = (& $adb devices 2>&1) | Select-String "$DEV\s+device"
  if ($s) { break }
  Start-Sleep -Seconds 2
}
if (-not $s) { L "no adb, giving up"; exit 1 }
L "adb up"

$t0 = Get-Date
$done = $false
$lastSS = ''
for ($i=0; $i -lt 120; $i++) {
  $bc = ((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
  $ba = ((& $adb shell getprop init.svc.bootanim 2>&1) -join '').Trim()
  $ss = ((& $adb shell pidof system_server 2>&1) -join '').Trim()
  $up = ((& $adb shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim()
  if ($ss -ne $lastSS) { L "system_server pid -> '$ss' (RESTART if changed)"; $lastSS = $ss }
  L ("t+{0,4}s up={1,4}s boot_completed='{2}' bootanim={3} ss={4}" -f [int]((Get-Date)-$t0).TotalSeconds, $up, $bc, $ba, $ss)
  if ($bc -eq '1') { $done = $true; break }
  Start-Sleep -Seconds 5
}

if ($done) {
  L "*** BOOT COMPLETED ***"
  L ("vold blocked? " + ((& $adb shell "logcat -d | grep -c 'WATCHDOG KILLING'" 2>&1) -join '').Trim() + " watchdog kills")
  foreach ($k in 'ro.build.version.release','ro.lineage.version','ro.vndk.version','sys.boot_completed') {
    L ("{0,-28} {1}" -f $k, ((& $adb shell getprop $k 2>&1) -join '').Trim())
  }
  & $adb shell logcat -d -v threadtime 2>&1 | Out-File F:\PN2Lineage\notes\38_logcat_ok.txt -Encoding utf8
} else {
  L "did NOT complete - capturing"
  & $adb shell logcat -d -v threadtime 2>&1 | Out-File F:\PN2Lineage\notes\38_logcat_fail.txt -Encoding utf8
  L ("watchdog kills: " + ((& $adb shell "logcat -d | grep -c 'WATCHDOG KILLING'" 2>&1) -join '').Trim())
}
L "=== done ==="

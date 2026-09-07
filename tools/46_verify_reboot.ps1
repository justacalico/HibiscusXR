# Reboot and verify BOTH fixes hold with no manual intervention:
#   - vendor manifest edit  -> vold no longer wedges -> boot completes
#   - /system/etc/modprobe.d -> stock init modprobe succeeds -> sound card exists
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\46_verify.log'
function L($m) { $s = ("[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m); Add-Content $log $s; Write-Host $s }
Set-Content $log "=== verify reboot $(Get-Date) ==="

L "rebooting"
& $adb reboot | Out-Null
Start-Sleep -Seconds 20
for ($i=0; $i -lt 150; $i++) {
  if ((& $adb devices 2>&1) | Select-String "$DEV\s+device") { break }
  Start-Sleep -Seconds 2
}
L "adb up"

$t0 = Get-Date; $ok = $false
for ($i=0; $i -lt 90; $i++) {
  $bc = ((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
  $up = ((& $adb shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim()
  L ("t+{0,4}s up={1,4}s boot_completed='{2}'" -f [int]((Get-Date)-$t0).TotalSeconds, $up, $bc)
  if ($bc -eq '1') { $ok = $true; break }
  Start-Sleep -Seconds 5
}

L "=========== RESULTS ==========="
L ("boot_completed : " + $(if ($ok) {'YES'} else {'NO'}))
L ("watchdog kills : " + ((& $adb shell "logcat -d | grep -c 'WATCHDOG KILLING'" 2>&1) -join '').Trim())
L ("sound modules  : " + ((& $adb shell "lsmod | grep -c snd_soc" 2>&1) -join '').Trim())
L ("sound card     : " + ((& $adb shell "cat /proc/asound/cards" 2>&1) -join ' ').Trim())
L ("audio_flinger  : " + ((& $adb shell "service check media.audio_flinger" 2>&1) -join '').Trim())
L ("audio_policy   : " + ((& $adb shell "service check media.audio_policy" 2>&1) -join '').Trim())
L ("audioserver crashes: " + ((& $adb shell "logcat -d -b crash | grep -c audioserver" 2>&1) -join '').Trim())

L "--- speaker amp (wsa881x) probe at early boot ---"
(& $adb shell "dmesg | grep -iE 'wsa881x|SpkrLeft|SpkrRight|wsa-max-devs|Sound card'" 2>&1) |
  ForEach-Object { Add-Content $log ("   " + $_) }

L "--- audio routing devices seen by the framework ---"
(& $adb shell "dumpsys audio | head -60" 2>&1) | ForEach-Object { Add-Content $log ("   " + $_) }
L "=== done ==="

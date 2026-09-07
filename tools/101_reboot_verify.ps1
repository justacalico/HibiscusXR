# Reboot, re-apply the suspend mitigation (stayon is runtime-only and does not
# survive a reboot), then report whether the deodexed Pico apps actually run.
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\101_verify.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== deodexed pico apps verify $(Get-Date) ==="

L "rebooting"
& $adb reboot 2>&1 | Out-Null
Start-Sleep -Seconds 40
& $adb wait-for-device 2>&1 | Out-Null

for ($i = 0; $i -lt 40; $i++) {
    $bc = ((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "boot_completed=$bc uptime=$(((& $adb shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim())s"

# suspend mitigation back on before anything else
& $adb shell "svc power stayon true" 2>&1 | Out-Null
L "stayon re-applied"

Start-Sleep -Seconds 20

L "--- packages registered ---"
(& $adb shell "pm list packages 2>/dev/null | grep -iE 'pvr|pico'" 2>&1) | ForEach-Object { L "   $_" }

L "--- dex problems (want 0) ---"
L ("   no-original-dex : " + ((& $adb shell "logcat -d | grep -c 'No original dex files found'" 2>&1) -join '').Trim())
L ("   ClassNotFound   : " + ((& $adb shell "logcat -d | grep -c ClassNotFoundException" 2>&1) -join '').Trim())

L "--- pico processes alive ---"
(& $adb shell "ps -A -o PID,UID,NAME | grep -iE 'pvr|pico'" 2>&1) | ForEach-Object { L "   $_" }

L "--- crashes ---"
(& $adb shell "logcat -d -b crash | grep -iE 'pvr|pico' | tail -8" 2>&1) | ForEach-Object { L "   $_" }

L ("system_server=" + ((& $adb shell pidof system_server 2>&1) -join '').Trim() +
   " pvrservice=" + ((& $adb shell pidof pvrservice 2>&1) -join '').Trim())
L "=== done ==="

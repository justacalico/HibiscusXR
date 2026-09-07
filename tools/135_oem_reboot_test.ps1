# Reboot so PackageManager scans the newly installed /oem-derived apps, then see
# whether PVRLauncher / PVRHome register and launch.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\135_oem.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== oem apps after reboot $(Get-Date) ==="

& $adb -s $DEV reboot 2>&1 | Out-Null
Start-Sleep -Seconds 45
& $adb -s $DEV wait-for-device 2>&1 | Out-Null
for ($i=0; $i -lt 40; $i++) {
    $bc = ((& $adb -s $DEV shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "boot_completed=$bc uptime=$(((& $adb -s $DEV shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim())s"
& $adb -s $DEV shell "svc power stayon true" 2>&1 | Out-Null
Start-Sleep -Seconds 15

L "--- pico packages now registered ---"
(& $adb -s $DEV shell "pm list packages 2>/dev/null | grep -iE 'pvr|pico'" 2>&1) | ForEach-Object { L "   $_" }

L "--- did the new ones scan ---"
foreach ($p in 'com.pvr.launcher','com.pvr.home','com.picovr.store','com.picovr.provision','com.pvr.tobservice') {
    $r = ((& $adb -s $DEV shell "pm path $p" 2>&1) -join '').Trim()
    L ("   {0,-24} {1}" -f $p, $(if ($r) { $r } else { 'NOT REGISTERED' }))
}

L "--- launching PVRLauncher ---"
& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "monkey -p com.pvr.launcher -c android.intent.category.LAUNCHER 1" 2>&1 | Out-Null
Start-Sleep -Seconds 10
L ("   pid: " + ((& $adb -s $DEV shell pidof com.pvr.launcher 2>&1) -join '').Trim())
(& $adb -s $DEV shell "dumpsys activity activities | grep -m2 -iE 'mResumedActivity'" 2>&1) | ForEach-Object { L "   $_" }
(& $adb -s $DEV shell "logcat -d | grep -iE 'launcher|AndroidRuntime|SIGSEGV' | tail -10" 2>&1) | ForEach-Object { L ("   " + (($_ -split ': ',2)[-1])) }
L "=== done ==="

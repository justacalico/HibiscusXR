# Revert forced landscape, reboot, and see whether VRShell survives UpdateLensInfo.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\150_portrait.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== VR test with native portrait $(Get-Date) ==="

& $adb -s $DEV push F:\PN2Lineage\tools\149_revert_landscape.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "chmod 755 /data/local/tmp/149_revert_landscape.sh; /system/xbin/su -c /data/local/tmp/149_revert_landscape.sh" 2>&1) |
    ForEach-Object { L "   $_" }

L "rebooting"
& $adb -s $DEV reboot 2>&1 | Out-Null
Start-Sleep -Seconds 45
& $adb -s $DEV wait-for-device 2>&1 | Out-Null
for ($i = 0; $i -lt 50; $i++) {
    $bc = ((& $adb -s $DEV shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "boot_completed=$bc"
Start-Sleep -Seconds 15

L ("wm size: " + ((& $adb -s $DEV shell "wm size" 2>&1) -join '').Trim())

L "--- launching VRShell ---"
& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 12
$pid1 = ((& $adb -s $DEV shell pidof com.pvr.vrshell 2>&1) -join '').Trim()
L "vrshell pid: $pid1  $(if ($pid1) { '*** SURVIVED ***' } else { 'died' })"

L "--- pvrservice display info ---"
(& $adb -s $DEV shell "logcat -d | grep -iE 'Override displayinfo|lensParameters|Override fov'" 2>&1) |
    ForEach-Object { L ("   " + (($_ -split ': ',2)[-1])) }

L "--- how far did it get ---"
(& $adb -s $DEV shell "logcat -d | grep -iE 'ConfigApi|psmvr|UnityNative|SIGSEGV|PvrClient|LoadingRes|Unity ' | tail -14" 2>&1) |
    ForEach-Object { L ("   " + (($_ -split ': ',2)[-1])) }
L "=== done ==="

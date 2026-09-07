# Reboot the port so the new ro.build.product takes effect, re-apply the suspend
# mitigation, then launch VRShell and report which platform profile pvrservice
# chose and how far the shell gets.
$adb = 'C:\adb\adb.exe'
$D   = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\126_vrshell.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== vrshell after product identity fix $(Get-Date) ==="

L "rebooting"
& $adb -s $D reboot 2>&1 | Out-Null
Start-Sleep -Seconds 45
& $adb -s $D wait-for-device 2>&1 | Out-Null
for ($i=0; $i -lt 40; $i++) {
    $bc = ((& $adb -s $D shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "boot_completed=$bc"
& $adb -s $D shell "svc power stayon true" 2>&1 | Out-Null
L "stayon re-applied"

L ("ro.build.product = " + ((& $adb -s $D shell getprop ro.build.product 2>&1) -join '').Trim())
L ("ro.product.model = " + ((& $adb -s $D shell getprop ro.product.model 2>&1) -join '').Trim())

Start-Sleep -Seconds 12
L "--- which platform profile did pvrservice choose ---"
(& $adb -s $D shell "logcat -d | grep -i 'Platform config file path' | tail -2" 2>&1) | ForEach-Object { L "   $_" }

L "--- launching VRShell ---"
& $adb -s $D shell "logcat -c" 2>&1 | Out-Null
& $adb -s $D shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 12
L ("vrshell pid   : " + ((& $adb -s $D shell pidof com.pvr.vrshell 2>&1) -join '').Trim())
L ("pvrservice pid: " + ((& $adb -s $D shell pidof pvrservice 2>&1) -join '').Trim())
L "--- last lines ---"
(& $adb -s $D shell "logcat -d | grep -iE 'ConfigApi|psmvr|UnityNative|VrApi|SIGSEGV|PvrClient|LoadingRes' | tail -18" 2>&1) |
    ForEach-Object { L ("   " + (($_ -split ': ',2)[-1])) }
L "=== done ==="

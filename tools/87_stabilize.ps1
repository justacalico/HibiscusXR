# The device is cycling: boot -> persistent pico app crash loop -> lockup ->
# reboot. There is not a reliable window to push 112 MB into that. Removing the
# broken apps is a handful of rm -rf and completes in the short window we do get,
# so do that first and install properly afterwards against a stable device.
$adb = 'C:\adb\adb.exe'
$sh  = 'F:\PN2Lineage\tools\84_remove_apps.sh'

for ($try = 1; $try -le 12; $try++) {
    Write-Host "attempt $try : waiting for device..."
    & $adb wait-for-device 2>&1 | Out-Null

    & $adb push $sh /data/local/tmp/remove_apps.sh 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { Start-Sleep -Seconds 3; continue }

    $out = & $adb shell "chmod 755 /data/local/tmp/remove_apps.sh; /system/xbin/su -c /data/local/tmp/remove_apps.sh" 2>&1 | Out-String
    Write-Host $out
    if ($out -match 'DONE') {
        Write-Host "REMOVED - device should stabilise now" -ForegroundColor Green
        break
    }
    Start-Sleep -Seconds 3
}

Write-Host "`n=== settling ==="
Start-Sleep -Seconds 20
& $adb wait-for-device 2>&1 | Out-Null
"uptime        : " + ((& $adb shell cut -d. -f1 /proc/uptime 2>&1) -join '') + "s"
"boot_completed: " + ((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
"system_server : " + ((& $adb shell pidof system_server 2>&1) -join '')
"pvrservice    : " + ((& $adb shell pidof pvrservice 2>&1) -join '')

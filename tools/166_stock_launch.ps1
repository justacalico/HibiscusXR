# Capture stock's VRShell2 launch sequence so it can be diffed against ours.
#
# Ours dies immediately after:
#   ConfigApi: psmvr_UpdateLensAndDisplayInfoFromVRService UpdateLensInfo
# with x28 trashed across a JNI return. Stock runs the identical app and the
# identical native stack, so the point where the two logs diverge is the defect.
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$out   = 'F:\PN2Lineage\notes\166_stock.log'

& $adb connect $STOCK 2>&1 | Out-Null
& $adb -s $STOCK shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
Start-Sleep -Seconds 2
& $adb -s $STOCK shell "logcat -c" 2>&1 | Out-Null
& $adb -s $STOCK shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 13

$all = & $adb -s $STOCK shell "logcat -d -v brief" 2>&1
Set-Content $out ($all -join "`n")
Write-Host ("captured " + $all.Count + " lines -> $out")

$p = ((& $adb -s $STOCK shell "pidof com.pvr.vrshell" 2>&1) -join '').Trim()
Write-Host ("stock vrshell pid: $p")
Write-Host ""
Write-Host "=== stock init sequence (VrApi / ConfigApi / VrServiceApi / PvrClient) ==="
$all | Where-Object { $_ -match 'VrApi|ConfigApi|VrServiceApi|PvrClient|psmvr|UnityNative|VRDataUtils|Paul|SensorConstruct' } |
    Select-Object -First 70 | ForEach-Object { Write-Host ("  " + $_) }

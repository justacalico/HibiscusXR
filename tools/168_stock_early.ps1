# Capture stock's EARLY VRShell2 init without chatty dropping it.
#
# The blobs are byte-identical to ours, so the "APP 2.8.6.16 vs lib 2.8.6.15 ->
# open replaceable aborted" line and the three "Undefined variable" errors are
# probably normal rather than something we introduced. Verify instead of assuming:
# grow the buffer, disable chatty pruning, and stream the launch.
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$out   = 'F:\PN2Lineage\notes\168_stock_early.log'
& $adb connect $STOCK 2>&1 | Out-Null

& $adb -s $STOCK shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
& $adb -s $STOCK shell "logcat -G 16M" 2>&1 | Out-Null
& $adb -s $STOCK shell "logcat -P ''" 2>&1 | Out-Null      # no chatty whitelist/blacklist pruning
Start-Sleep -Seconds 2
& $adb -s $STOCK shell "logcat -c" 2>&1 | Out-Null
& $adb -s $STOCK shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 10

$all = & $adb -s $STOCK shell "logcat -d -v brief" 2>&1
Set-Content $out ($all -join "`n")
Write-Host ("captured " + $all.Count + " lines")
Write-Host ""
Write-Host "=== does STOCK show the same version-mismatch + config errors? ==="
$keys = 'open replaceable|APP version|lib version|Undefined variable|InitServiceClient|UpdateLensInfo|UpdateDisplayInfo|Initialize Global Configs|pvr_OnLoad|pvr_Init|6DofReset|NameNotFound'
$all | Where-Object { $_ -match $keys } | ForEach-Object { Write-Host ("  " + $_) }

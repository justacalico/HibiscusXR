param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
& $adb connect $Dev 2>&1 | Out-Null
$sh = @(
  'logcat -c',
  'am force-stop com.pvr.vrshell',
  'sleep 2',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 25',
  'echo "=== CVService crash header ==="',
  'logcat -d | grep -E "signal |Abort message|Cause:|CVService|libPvr_UnitySDKCV" | grep -vE "^.*#[0-9]" | head -12',
  'echo',
  'echo "=== everything vrshell logged, tail ==="',
  'P=$(pidof com.pvr.vrshell)',
  'logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo" | tail -35',
  'echo',
  'echo "=== errors from the shell pid ==="',
  'logcat -d | grep " $P " | grep -E " [EWF] " | grep -viE "Override displayinfo" | tail -20'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_ld.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_ld.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_ld.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

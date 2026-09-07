param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
& $adb connect $Dev 2>&1 | Out-Null
$sh = @(
  'logcat -c',
  'am force-stop com.pvr.vrshell',
  'sleep 2',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 22',
  'P=$(pidof com.pvr.vrshell)',
  'echo "=== VRShell startup, unfiltered, first 60 ==="',
  'logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo|FrameAnimation|ControllerClient" | head -60',
  'echo',
  'echo "=== does anything reference pvr_manager / ConfigurationService ==="',
  'logcat -d | grep -iE "pvr_manager|IPvrManagerService|ConfigurationService|IConfigService" | tail -10',
  'echo',
  'echo "=== is com.pvr.configuration alive and did it try to register ==="',
  'pidof com.pvr.configuration',
  'logcat -d | grep -iE "com.pvr.configuration" | tail -8'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_ww.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_ww.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_ww.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

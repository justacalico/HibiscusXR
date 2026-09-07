param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
& $adb connect $Dev 2>&1 | Out-Null
$sh = @(
  'logcat -c',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 18',
  'echo "=== register dump at the fault ==="',
  'logcat -d | grep -E "    r[0-9]|    ip |Cause:|fault addr|>>> com.picovr" | head -14',
  'echo',
  'echo "=== what does stock have running that we do not ==="',
  'ps -A | grep -iE "picovr|pvr|hummingbird" | head -20',
  'echo',
  'echo "=== is com.pvr.home installed here? ==="',
  'pm list packages | grep -iE "pvr.home|picovr.home|launcher" ',
  'echo',
  'echo "=== binder services ==="',
  'service list 2>/dev/null | grep -iE "pvr|picovr|cv" | head'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_regs.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_regs.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_regs.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

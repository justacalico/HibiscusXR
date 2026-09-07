param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
& $adb connect $Dev 2>&1 | Out-Null
$sh = @(
  'echo "=== what activities does com.pvr.home expose? ==="',
  'dumpsys package com.pvr.home | grep -A2 -iE "Activity Resolver|android.intent.action.MAIN" | head -20',
  'echo',
  'echo "=== launch it ==="',
  'logcat -c',
  'am start -n com.pvr.home/.RecActivity 2>&1 | head -5',
  'sleep 20',
  'echo "  pid $(pidof com.pvr.home)"',
  'echo "=== focus ==="',
  'dumpsys window | grep mCurrentFocus',
  'echo "=== errors from home ==="',
  'P=$(pidof com.pvr.home); logcat -d | grep " $P " | grep -E " [EWF] " | grep -viE "Override displayinfo|Undefined variable" | tail -18',
  'echo "=== did it enter VR mode / render? ==="',
  'logcat -d | grep -iE "EnterVrMode|TimeWarp|Unity|Boundary|Seethrough" | tail -12'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_home.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_home.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_home.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_home.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_home.png F:\PN2Lineage\notes\pn2_home.png 2>&1 | Out-Null

# Launch the see-through app by hand. Deliberately NOT setting
# persist.pvrcon.seethrough.enable=1 - that makes the system relaunch the flow
# forever and bootloops the 2D loading screen.
#
# Also capture the crash that is still looping: the BitTube guard held (no
# libpvrserviceclient frames left) but something else is now SIGSEGVing.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\367_st.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== manual see-through launch $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "=== what is still crashing? ==="',
  'logcat -d | grep -A8 "backtrace:" | grep -E "#0[0-4] " | head -10',
  'echo "  --- and which process ---"',
  'logcat -d | grep -E ">>> .* <<<" | tail -3',
  'echo',
  'echo "=== launch see-through ==="',
  'logcat -c',
  'am start -n com.pvr.seethrough.setting/.MainActivity 2>&1 | head -3',
  'sleep 30',
  'S=$(pidof com.pvr.seethrough.setting)',
  'echo "  pid $S  threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"',
  'dumpsys window 2>/dev/null | grep -m1 mCurrentFocus',
  'echo',
  'echo "=== did it reach VR mode / open the cameras? ==="',
  'logcat -d | grep -iE "EnterVrMode|TimeWarp|aircamera|AIRService|SeeThrough|StartCameraInternal" | grep -viE "LockBuffer" | tail -12',
  'echo',
  'echo "=== app log tail ==="',
  'logcat -d | grep " $S " | grep -viE "chatty|Undefined variable|avc:|Override displayinfo" | tail -22',
  'echo',
  'echo "=== tracking still alive? ==="',
  'echo "  kLost=$(logcat -d | grep -c kLostDialog)"',
  'logcat -d | grep -iE "getTrackingDataExt position" | tail -2'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_st4.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_st4.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_st4.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_st5.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_st5.png F:\PN2Lineage\notes\pn2_st5.png 2>&1 | Out-Null

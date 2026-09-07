# Launch the see-through / 6DoF boundary calibration app now that 6DoF tracking
# is confirmed working. Previously this could never have succeeded: the tracker
# had no usable pose, so the calibration flow had nothing to calibrate against.
#
# persist.pvrcon.seethrough.enable is 0 on ours and 1 on stock - set it to match
# before launching, since the app and the shell both branch on it.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\364_seethrough.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== see-through app $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- what the app exposes ---"',
  'dumpsys package com.pvr.seethrough.setting | grep -B1 -A3 "android.intent.action.MAIN" | head -20',
  'echo',
  'echo "--- match stock: seethrough enabled ---"',
  'setprop persist.pvrcon.seethrough.enable 1',
  'echo "  persist.pvrcon.seethrough.enable = $(getprop persist.pvrcon.seethrough.enable)"',
  'echo',
  'echo "--- airservice is what feeds the camera passthrough; make sure it is up ---"',
  'start airservice 2>/dev/null',
  'sleep 3',
  'echo "  airservice pid $(pidof airservice)"',
  'echo',
  'logcat -c',
  'echo "--- launch ---"',
  'am start -n com.pvr.seethrough.setting/com.unity3d.player.UnityPlayerNativeActivityPico 2>&1 | head -3',
  'sleep 20',
  'P=$(pidof com.pvr.seethrough.setting)',
  'echo "  pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"',
  'echo "--- focus ---"',
  'dumpsys window | grep mCurrentFocus',
  'echo "--- crashes ---"',
  'logcat -d | grep -E "E CRASH|signal 11|FATAL" | head -6',
  'echo "  (empty = no crash)"',
  'echo "--- camera / passthrough ---"',
  'logcat -d | grep -iE "airservice|aircamera|seethrough|passthrough|QVRServiceCamDeviceHAL3|camera" | grep -viE "LockBuffer" | tail -14',
  'echo "--- app log ---"',
  'logcat -d | grep " $P " | grep -viE "chatty|Undefined variable|avc:" | tail -20'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_st2.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_st2.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_st2.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_st.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_st.png F:\PN2Lineage\notes\pn2_st.png 2>&1 | Out-Null

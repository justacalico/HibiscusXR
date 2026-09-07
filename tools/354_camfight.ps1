# Does airservice steal the tracking cameras from the 6DoF tracker?
#
# The compositor is alive and rendering (TimeWarp running, home environment drawn)
# but the pose feed freezes on one buffer a few seconds in, and the QVR tracker
# thread exits with QVR_CAM_DEVICE_STOPPING. QVR allows exactly ONE VR-mode owner,
# and airservice retries camera opens in a loop. If it is the thief, stopping it
# should keep tracking alive.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\354_camfight.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== airservice camera contention test $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- who else is holding QVR / cameras ---"',
  'ps -A | grep -iE "airservice|qvrservicetest|seethrough" | awk "{print \"  \"\$1\" \"\$NF}"',
  'echo "--- stop the competition ---"',
  'stop airservice 2>/dev/null; pkill -f qvrservicetest 2>/dev/null',
  'am force-stop com.pvr.vrshell',
  'sleep 3',
  'echo "--- restart pvrservice so the tracker starts clean ---"',
  'stop pvrservice; sleep 3; start pvrservice; sleep 8',
  'echo "  pvrservice pid $(pidof pvrservice)"',
  'logcat -c',
  'echo "--- launch the shell ---"',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'echo "--- watch tracking for 60s ---"',
  'i=0',
  'while [ $i -lt 6 ]; do',
  '  sleep 10',
  '  BP=$(logcat -d | grep -c "Bad Pose")',
  '  TR=$(logcat -d | grep -c "Ending Thread VRTracker")',
  '  CS=$(logcat -d | grep -c "QVR_CAM_DEVICE_STOPPING")',
  '  echo "  t+$(( (i+1)*10 ))s  BadPose=$BP  trackerExit=$TR  camStopping=$CS"',
  '  i=$((i+1))',
  'done',
  'echo "--- tracker thread history ---"',
  'logcat -d | grep -iE "VRTracker|6DOF|QVR_CAM|Starting Thread|Ending Thread" | tail -14',
  'echo "--- who opened the cameras ---"',
  'logcat -d | grep -iE "QVRServiceCamDeviceHAL3|camera.*open|CamDevice" | tail -8'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_cf.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_cf.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_cf.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_cf.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_cf.png F:\PN2Lineage\notes\pn2_cf.png 2>&1 | Out-Null

# Put the x27-based patch back.
#
# The x24 experiment is answered: with CVService out of the way VRShell gets all
# the way into pvr_EnterVrMode and then crashes at 0x5ff70, 0x14 past the x24
# trampoline's resume point. patched2.so (sub x28, x27, #0x148) survives that
# stretch, so x27 was the intact register and the x24 recomputation is wrong.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\350_revert.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== revert to x27 patch $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

& $adb -s $Dev shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.patched2.so /data/local/tmp/p2.so 2>&1 | Out-Null

$sh = @(
  'T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so',
  'mount -o rw,remount /system',
  'cp /data/local/tmp/p2.so "$T.new"',
  'chmod 644 "$T.new"; chown root:root "$T.new"',
  'chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null',
  'mv -f "$T.new" "$T"',
  'sync',
  'md5sum "$T" /data/local/tmp/p2.so',
  'echo "--- controller service stays disabled for now ---"',
  'pm list packages -d | grep cvcontroller',
  'stop pvrservice; sleep 3; start pvrservice; sleep 6',
  'echo "--- launch ---"',
  'logcat -c',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 30',
  'P=$(pidof com.pvr.vrshell)',
  'echo "  pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"',
  'echo "--- crashes? ---"',
  'logcat -d | grep -E "E CRASH|signal 11" | head -6',
  'echo "  (empty = no crash)"',
  'echo "--- vr mode + compositor ---"',
  'logcat -d | grep -iE "EnterVrMode|TimeWarp|SubmitFrame|frame rate|Compositor" | tail -14',
  'echo "--- tracking ---"',
  'BP=$(logcat -d | grep -c "Bad Pose"); KL=$(logcat -d | grep -c kLostDialog); TS=$(logcat -d | grep -c "trackingstate = 0x0")',
  'echo "  BadPose=$BP kLost=$KL ts0=$TS"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_rev.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_rev.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_rev.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_x27.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_x27.png F:\PN2Lineage\notes\pn2_x27.png 2>&1 | Out-Null

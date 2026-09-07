# Install the x24-based patch and see whether VR mode now initialises properly.
# If x24 was the intact register, pvr_EnterVrMode finally gets the RIGHT struct
# pointer and should set up tracking correctly instead of writing 0x20 off.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\329_x24.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== x24 patch test $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

& $adb -s $Dev shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.x24.so /data/local/tmp/x24.so 2>&1 | Out-Null

$sh = @(
  'T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so',
  'mount -o rw,remount /system',
  'cp /data/local/tmp/x24.so "$T.new"',
  'chmod 644 "$T.new"; chown root:root "$T.new"',
  'chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null',
  'mv -f "$T.new" "$T"',
  'sync',
  'md5sum "$T" /data/local/tmp/x24.so',
  'echo "--- launching ---"',
  'logcat -c',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 25',
  'echo "  pid $(pidof com.pvr.vrshell)"',
  'echo "--- did pvrservice finally ask QVR? ---"',
  'logcat -d | grep -iE "QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create" | tail -5',
  'echo "--- does pvrservice load the qvr client now? ---"',
  'grep -oE "libqvr[^ ]*\.so" /proc/$(pidof pvrservice)/maps 2>/dev/null | sort -u',
  'echo "--- compositor ---"',
  'echo "  BadPose  = $(logcat -d | grep -c \"Bad Pose\")"',
  'echo "  kLost    = $(logcat -d | grep -c kLostDialog)"',
  'echo "  ts0      = $(logcat -d | grep -c \"trackingstate = 0x0\")"',
  'echo "--- hmdInfo (did EnterVrMode read sane values?) ---"',
  'logcat -d | grep -iE "hmdInfo|EnterVrMode" | head -8'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_x24.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_x24.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_x24.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

# Copy libqvrservice_client.so into /system so pvrservice can load it.
#
# EXACTLY the same failure as libcdsprpc.so: the library lives only in /vendor/lib*,
# pvrservice runs from /system/bin, and Android 10's Treble namespace separation
# forbids a /system process loading from /vendor/lib. Android 8.1 permitted it,
# which is why stock works and we do not.
#
# Chain this unblocks:
#   dlopen fails -> QVRServiceClient_Create returns NULL
#   -> "QVR Serivce reported VR not supported"
#   -> pvrservice never starts 6DoF
#   -> _trackingDataExt +0x9c stays 0
#   -> SDK trackingstate = 0x0,0x0
#   -> BoundarySystem -> kLostDialog, compositor rejects every pose -> black screen
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\331_qvrclient.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== libqvrservice_client -> /system $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'mount -o rw,remount /system',
  '# check what it needs before installing',
  'echo "--- deps of the vendor copy ---"',
  'cp -f /vendor/lib64/libqvrservice_client.so /system/lib64/libqvrservice_client.so',
  'cp -f /vendor/lib/libqvrservice_client.so   /system/lib/libqvrservice_client.so',
  '# the camera client is the same story - pvrservice/airservice need it too',
  'cp -f /vendor/lib64/libqvrcamera_client.so  /system/lib64/libqvrcamera_client.so 2>/dev/null',
  'cp -f /vendor/lib/libqvrcamera_client.so    /system/lib/libqvrcamera_client.so 2>/dev/null',
  'chmod 644 /system/lib64/libqvr*.so /system/lib/libqvr*.so',
  'chown root:root /system/lib64/libqvr*.so /system/lib/libqvr*.so',
  'chcon u:object_r:system_lib_file:s0 /system/lib64/libqvr*.so /system/lib/libqvr*.so 2>/dev/null',
  'sync',
  'ls -l /system/lib64/libqvrservice_client.so /system/lib64/libqvrcamera_client.so',
  'echo "--- restart pvrservice so it re-asks QVR ---"',
  'stop pvrservice; sleep 2; start pvrservice; sleep 6',
  'echo "--- launch the shell ---"',
  'am force-stop com.pvr.vrshell',
  'sleep 2',
  'logcat -c',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 25',
  'echo "  pid $(pidof com.pvr.vrshell)"',
  'echo "--- QVR verdict now ---"',
  'logcat -d | grep -iE "QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create" | tail -6',
  'echo "--- does pvrservice map the client now? ---"',
  'grep -oE "libqvr[^ ]*\.so" /proc/$(pidof pvrservice)/maps 2>/dev/null | sort -u',
  'echo "--- compositor ---"',
  'BP=$(logcat -d | grep -c "Bad Pose"); KL=$(logcat -d | grep -c kLostDialog); TS=$(logcat -d | grep -c "trackingstate = 0x0")',
  'echo "  BadPose=$BP  kLost=$KL  ts0=$TS"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_qvrc.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_qvrc.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_qvrc.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

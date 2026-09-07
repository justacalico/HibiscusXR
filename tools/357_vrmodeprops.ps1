# The pose values are valid but poseStatus is 0. Stock carries two properties we
# never set - pvr.display.vrmode.enable=1 and sys.pvr.vrservice.state=2 - which
# look like exactly what PvrManagerService would publish. If pvrservice gates the
# pose status bits on them, setting them by hand should make trackingstate
# non-zero without needing the framework service at all.
#
# Set before restarting pvrservice, since it may only read them at startup.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\357_vrmodeprops.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== vr-mode property test $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- set the properties stock has and we do not ---"',
  'setprop pvr.display.vrmode.enable 1',
  'setprop sys.pvr.vrservice.state 2',
  'setprop pvr.running.app.3dof false',
  'setprop pvr.controller.mode 1',
  'setprop pxr.2dtovr.disable 0',
  'setprop pvr.2d_screen.reposition 0',
  'setprop pxr.service.6dof.restarted false',
  'setprop sys.pvr.floorheight -0.5286096',
  'setprop pvr.sleep.mode 0',
  'for p in pvr.display.vrmode.enable sys.pvr.vrservice.state pxr.service.6dof.restarted; do',
  '  echo "  $p = $(getprop $p)"',
  'done',
  'echo "--- restart pvrservice so it picks them up ---"',
  'am force-stop com.pvr.vrshell',
  'stop pvrservice; sleep 3; start pvrservice; sleep 8',
  'echo "  pvrservice pid $(pidof pvrservice)"',
  'logcat -c',
  'echo "--- launch the shell ---"',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 25',
  'echo "--- trackingstate now ---"',
  'logcat -d | grep -oE "trackingstate = 0x[0-9a-f]+,0x[0-9a-f]+" | sort | uniq -c | tail -5',
  'echo "  (anything other than 0x0,0x0 is the win)"',
  'echo "--- compositor ---"',
  'BP=$(logcat -d | grep -c "Bad Pose"); KL=$(logcat -d | grep -c kLostDialog)',
  'echo "  BadPose=$BP  kLost=$KL"',
  'echo "--- live pose (should still be valid) ---"',
  'logcat -d | grep -iE "getTrackingDataExt position|svrHeadPoseStateOutput" | tail -3'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_vm.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_vm.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_vm.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_vm.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_vm.png F:\PN2Lineage\notes\pn2_vm.png 2>&1 | Out-Null

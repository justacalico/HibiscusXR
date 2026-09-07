# Install the BitTube null guard and bring the controller service back.
#
# Copy to a temp name and mv into place - writing over a mapped .so gives SIGBUS
# in every process that has it open.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\366_bittube.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== BitTube null guard $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\notes\pvrsc32.patched.so /data/local/tmp/pvrsc32p.so 2>&1 | Out-Null

$sh = @(
  'T=/system/lib/libpvrserviceclient.so',
  'mount -o rw,remount /system',
  'echo "--- back up the original once ---"',
  '[ -f /data/local/tmp/pvrsc32.orig.so ] || cp "$T" /data/local/tmp/pvrsc32.orig.so',
  'cp /data/local/tmp/pvrsc32p.so "$T.new"',
  'chmod 644 "$T.new"; chown root:root "$T.new"',
  'chcon u:object_r:system_lib_file:s0 "$T.new" 2>/dev/null',
  'mv -f "$T.new" "$T"',
  'sync',
  'md5sum "$T" /data/local/tmp/pvrsc32p.so',
  'echo',
  'echo "--- clean slate: healthy pvrservice, controller service on ---"',
  'am force-stop com.pvr.vrshell',
  'stop pvrservice; sleep 3; start pvrservice; sleep 6',
  'P=$(pidof pvrservice); echo "  pvrservice pid $P"',
  'pm enable com.picovr.picovrlib.cvcontroller 2>&1 | tail -1',
  'sleep 2',
  'logcat -c',
  'echo',
  'echo "--- launch the shell (this is what binds the controller service) ---"',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'echo "--- watch 60s ---"',
  'i=0',
  'while [ $i -lt 6 ]; do',
  '  sleep 10',
  '  SEGV=$(logcat -d | grep -c "exited due to signal 11")',
  '  CV=$(pidof com.picovr.picovrlib.cvcontroller:RemoteService)',
  '  echo "  t+$(( (i+1)*10 ))s  segv=$SEGV  cvPid=${CV:-none}  vrshell=$(pidof com.pvr.vrshell)"',
  '  i=$((i+1))',
  'done',
  'echo',
  'echo "--- pvrservice health ---"',
  'P=$(pidof pvrservice)',
  'echo "  contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)  zombies=$(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P "\$2==p" | grep -c Z)"',
  'echo "--- tracking + compositor ---"',
  'echo "  BadPose=$(logcat -d | grep -c \"Bad Pose\")  kLost=$(logcat -d | grep -c kLostDialog)"',
  'V=$(pidof com.pvr.vrshell); echo "  vrshell threads=$(ls /proc/$V/task 2>/dev/null | wc -l)"',
  'echo "--- controller service alive and talking? ---"',
  'logcat -d | grep -iE "ControllerClient|controller_type|bind service fail" | tail -6',
  'echo "--- any remaining getPvrService crash ---"',
  'logcat -d | grep -A4 "backtrace:" | grep -E "libpvrserviceclient|BitTube" | head -4',
  'echo "  (empty = the guard held)"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_bt3.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_bt3.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_bt3.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_bt.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_bt.png F:\PN2Lineage\notes\pn2_bt.png 2>&1 | Out-Null

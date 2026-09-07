# Put the controller service back now that tracking is confirmed working.
#
# I disabled it to break the cascade where its crash loop wedged pvrservice via
# fork-vs-jemalloc. That also removed the pointer, which is why the home app has
# nothing to interact with and why the "bind service fail" toast appears.
#
# Re-enable with pvrservice already healthy and watch whether it settles or starts
# crash-looping again. Do NOT restart pvrservice afterwards - tracking is live and
# I do not want to disturb it.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\363_recv.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== re-enable controller service $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- pvrservice health before ---"',
  'P=$(pidof pvrservice)',
  'echo "  pid $P  malloc-contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)"',
  'logcat -c',
  'echo "--- re-enable ---"',
  'pm enable com.picovr.picovrlib.cvcontroller 2>&1 | tail -1',
  'sleep 3',
  'am startservice com.picovr.picovrlib.cvcontroller/.cvcontrollerlib.CVControllerService >/dev/null 2>&1',
  'echo "--- watch 40s: does it settle or crash-loop? ---"',
  'i=0',
  'while [ $i -lt 4 ]; do',
  '  sleep 10',
  '  C=$(logcat -d | grep -c "exited due to signal 11")',
  '  R=$(logcat -d | grep -c "registerClient")',
  '  echo "  t+$(( (i+1)*10 ))s  segv=$C  registerClient=$R  cvPid=$(pidof com.picovr.picovrlib.cvcontroller:RemoteService)"',
  '  i=$((i+1))',
  'done',
  'echo "--- pvrservice health after ---"',
  'P=$(pidof pvrservice)',
  'echo "  pid $P  malloc-contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)"',
  'echo "  zombie am children=$(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P \x27$2==p && $3 ~ /Z/\x27 | wc -l)"',
  'echo "--- is tracking still good? ---"',
  'echo "  BadPose=$(logcat -d | grep -c \"Bad Pose\")  kLost=$(logcat -d | grep -c kLostDialog)"',
  'logcat -d | grep -iE "getTrackingDataExt position" | tail -2',
  'echo "--- controller seen? ---"',
  'logcat -d | grep -iE "ControllerClient|controller_type|bind service" | tail -6'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_recv.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_recv.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_recv.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

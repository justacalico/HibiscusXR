# Break the cascade to find out what is cause and what is symptom.
#
#   CVService crashes -> respawns ~1/s -> each one registerClient/binderDied on
#   pvrservice -> pvrservice system()s "am startservice ..." from a binder thread
#   -> fork() grabs every jemalloc arena lock while another binder thread is in
#   free() -> pvrservice wedges -> VRShell blocks forever in registerClient.
#
# If VRShell renders with the controller service disabled, CVService is the only
# thing left in the way and everything else is downstream of it.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\349_nocv.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== VRShell without the controller service $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'echo "--- stop the crash loop ---"',
  'pm disable com.picovr.picovrlib.cvcontroller 2>&1 | tail -1',
  'am force-stop com.picovr.picovrlib.cvcontroller',
  'sleep 2',
  'echo "--- restart pvrservice clean ---"',
  'stop pvrservice; sleep 3; start pvrservice; sleep 6',
  'echo "  pvrservice pid $(pidof pvrservice)"',
  'echo "--- launch the shell ---"',
  'logcat -c',
  'am force-stop com.pvr.vrshell',
  'sleep 2',
  'am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1',
  'sleep 30',
  'P=$(pidof com.pvr.vrshell)',
  'echo "  vrshell pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"',
  'echo "--- did it get past registerClient? ---"',
  'logcat -d | grep -iE "EnterVrMode|hmdInfo.lensSeparation|TimeWarp|pvr_Init|VrApi" | tail -12',
  'echo "--- is the main thread still stuck in binder? ---"',
  'debuggerd -b $P 2>&1 | grep -A3 "^\"com.pvr.vrshell\"" | head -5',
  'echo "--- pvrservice zombies / health ---"',
  'echo "  zombies=$(ps -A | grep -c \" Z \")"',
  'echo "  malloc-contended threads=$(debuggerd -b $(pidof pvrservice) 2>&1 | grep -c je_malloc_mutex)"',
  'echo "--- compositor ---"',
  'echo "  BadPose=$(logcat -d | grep -c \"Bad Pose\")  kLost=$(logcat -d | grep -c kLostDialog)"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_nocv.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_nocv.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_nocv.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }
& $adb -s $Dev shell "screencap -p /sdcard/pn2_nocv.png" 2>&1 | Out-Null
& $adb -s $Dev pull /sdcard/pn2_nocv.png F:\PN2Lineage\notes\pn2_nocv.png 2>&1 | Out-Null

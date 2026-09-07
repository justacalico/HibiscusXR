# Install the x28-preserving trampoline into VRShell2's bundled SDK and retest.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\213_x28patch.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== x28 trampoline test $(Get-Date) ==="

& $adb connect $Dev 2>&1 | Out-Null
& $adb -s $Dev shell "am force-stop com.pvr.vrshell" 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\notes\vrshell_lib\libPvr_UnitySDK.patched.so /data/local/tmp/libPvr_patched.so 2>&1 |
    Select-Object -Last 1 | ForEach-Object { L "   $_" }

$sh = @(
  'T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so',
  'mount -o rw,remount /system',
  '[ -f "$T.orig" ] || cp -a "$T" "$T.orig"',
  'cp /data/local/tmp/libPvr_patched.so "$T.new"',
  'chmod 644 "$T.new"; chown root:root "$T.new"',
  'chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null',
  'mv -f "$T.new" "$T"',
  'sync',
  'md5sum "$T" /data/local/tmp/libPvr_patched.so'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_x28.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_x28.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_x28.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "--- launching VRShell ---"
& $adb -s $Dev shell "logcat -c" 2>&1 | Out-Null
& $adb -s $Dev shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 18

$p = ((& $adb -s $Dev shell "pidof com.pvr.vrshell" 2>&1) -join '').Trim()
L ("   vrshell pid: $p  " + $(if ($p) { '*** SURVIVED ***' } else { 'died' }))

$all = & $adb -s $Dev shell "logcat -d -v brief" 2>&1
Set-Content 'F:\PN2Lineage\notes\213_render.log' ($all -join "`n")

L "--- how far did it get ---"
$all | Where-Object { $_ -match 'EnterVrMode|hmdInfo|Cleared JNI|InitRenderThread|TimeWarp|Distortion|CRASH|Fatal signal|getPowerLevel|WarpSwap|DIATW' } |
    Select-Object -First 30 | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

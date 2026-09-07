# Tombstone for the NEW failure: pvrservice segfaults in a binder thread when
# VRShell connects, now that the JNI trampoline crash no longer kills the client
# first. pvrservice is the one process we LD_PRELOAD libshim_pvr.so into, so our
# own shim is a prime suspect.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\173_pvr_tomb.log'
function L($m) { Add-Content $log $m; Write-Host $m }
Set-Content $log "=== pvrservice tombstone ==="

L "--- is our shim preloaded into pvrservice? ---"
$p = ((& $adb -s $DEV shell "pidof pvrservice" 2>&1) -join '').Trim()
L "  pvrservice pid: $p"
if ($p) {
  (& $adb -s $DEV shell "su -c 'cat /proc/$p/environ | tr \047\\0\047 \047\\n\047 | grep -i preload'" 2>&1) | ForEach-Object { L "  env: $_" }
  (& $adb -s $DEV shell "su -c 'grep -oE ""/system/lib64/libshim_pvr.so"" /proc/$p/maps | head -1'" 2>&1) | ForEach-Object { L "  map: $_" }
}

L ""
L "--- newest tombstone ---"
$newest = ((& $adb -s $DEV shell "su -c 'ls -t /data/tombstones/tombstone_* 2>/dev/null | head -1'" 2>&1) -join '').Trim()
L "file: $newest"
if ($newest) {
  (& $adb -s $DEV shell "su -c 'cat $newest'" 2>&1) | Select-Object -First 60 | ForEach-Object { L "   $_" }
}

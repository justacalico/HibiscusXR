# Capture the real tombstone for the Pico VR crash.
#
# So far the diagnosis has run off a logcat line ("UpdateLensInfo" then "Fatal
# signal 11 ... fault addr 0x10"). That names the last thing that LOGGED, not the
# code that faulted. Disassembly of both PvrClientJava Java-bridge functions shows
# they null-check everything, so the fault is elsewhere. The tombstone gives the
# faulting library + offset + registers directly.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\158_tombstone.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== tombstone capture $(Get-Date) ==="

L ("wm size: " + ((& $adb -s $DEV shell "wm size" 2>&1) -join '').Trim())
L ("device : " + ((& $adb -s $DEV shell "getprop ro.product.device" 2>&1) -join '').Trim())
L ("hwrot  : " + ((& $adb -s $DEV shell "getprop ro.sf.hwrotation" 2>&1) -join '').Trim())

# note the newest existing tombstone so we can tell the new one apart
$before = ((& $adb -s $DEV shell "su -c 'ls /data/tombstones/ 2>/dev/null'" 2>&1) -join ' ')
L "tombstones before: $before"

L "--- launching VRShell ---"
& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 14

$pid1 = ((& $adb -s $DEV shell pidof com.pvr.vrshell 2>&1) -join '').Trim()
L "vrshell pid: $pid1  $(if ($pid1) { 'SURVIVED' } else { 'died' })"

L "--- crash lines from logcat ---"
(& $adb -s $DEV shell "logcat -d -b crash,main | grep -iE 'Fatal signal|backtrace|pc 0000|#0[0-9]|DEBUG|signal 11|Cause' | tail -40" 2>&1) |
    ForEach-Object { L ("   " + $_) }

L "--- newest tombstone ---"
$newest = ((& $adb -s $DEV shell "su -c 'ls -t /data/tombstones/ 2>/dev/null | head -1'" 2>&1) -join '').Trim()
L "file: $newest"
if ($newest) {
    (& $adb -s $DEV shell "su -c 'cat /data/tombstones/$newest'" 2>&1) |
        Select-Object -First 90 | ForEach-Object { L ("   " + $_) }
}
L "=== done ==="

# Revert the ART patch and retest.
#
# The trampoline patch (mov sp,x28 -> mov sp,x29) was made BEFORE the
# public.libraries.txt fix, when every Pico dlopen returned null and the process
# was already running on corrupted state. That crash may not exist at all now. If
# VRShell gets equally far on stock ART, the patch comes out - shipping a modified
# runtime to work around a problem that no longer exists is not defensible.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\207_revert_art.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== revert ART patch $(Get-Date) ==="

$sh = @(
  'T=/system/apex/com.android.runtime.release/lib64/libart.so',
  'mount -o rw,remount /system',
  'if [ -f "$T.orig" ]; then cp -a "$T.orig" "$T"; echo "reverted to stock ART"; else echo "NO BACKUP"; fi',
  'sync',
  'md5sum "$T" "$T.orig"'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_revart.sh', $sh + "`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_revart.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_revart.sh'" 2>&1) | ForEach-Object { L "   $_" }
L "rebooting"
& $adb -s $DEV reboot 2>&1 | Out-Null

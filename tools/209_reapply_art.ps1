# Re-apply the ART trampoline patch.
#
# Proven necessary: on stock ART, VRShell dies at art_quick_generic_jni_trampoline
# +176 (fault addr 0x10) immediately after UpdateLensInfo, exactly as before, and
# never reaches the render thread. The public.libraries.txt fix did not touch this
# failure - the two are independent.
#
# It stays a workaround rather than a repair, so it is documented and reversible
# via the .orig backup that sits beside it on the device.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\209_reapply.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== re-apply ART patch $(Get-Date) ==="

& $adb connect $Dev 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\notes\libart-patched.so /data/local/tmp/libart-patched.so 2>&1 |
    Select-Object -Last 1 | ForEach-Object { L "   $_" }

# write to a temp path then mv: never overwrite a mapped .so in place, that
# invalidates its pages and the next instruction fetch SIGBUSes (learned the hard way)
$sh = @(
  'T=/system/apex/com.android.runtime.release/lib64/libart.so',
  'mount -o rw,remount /system',
  '[ -f "$T.orig" ] || cp -a "$T" "$T.orig"',
  'cp /data/local/tmp/libart-patched.so "$T.new"',
  'chmod 644 "$T.new"; chown root:root "$T.new"',
  'chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null',
  'mv -f "$T.new" "$T"',
  'sync',
  'md5sum "$T" /data/local/tmp/libart-patched.so'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_reart.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_reart.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_reart.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting (wireless should come back by itself)"
& $adb -s $Dev reboot 2>&1 | Out-Null

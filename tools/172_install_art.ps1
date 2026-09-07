# Install the patched libart and test whether Pico VR apps survive.
#
# One instruction changed in art_quick_generic_jni_trampoline:
#   mov sp, x28   ->   mov sp, x29
# Both hold the same frame base; x29 survives the native call in the tombstone
# while x28 comes back zero. Semantically equivalent, so the risk is low, but this
# is ART: keep a backup on the device and remember that fastboot + the built
# images are the recovery path if the device stops booting.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\172_art.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== patched libart install $(Get-Date) ==="

$TGT = '/system/apex/com.android.runtime.release/lib64/libart.so'

L "pushing patched libart"
& $adb -s $DEV push F:\PN2Lineage\notes\libart-patched.so /data/local/tmp/libart-patched.so 2>&1 | Select-Object -Last 1 | ForEach-Object { L "   $_" }

$script = @'
set -e
TGT=/system/apex/com.android.runtime.release/lib64/libart.so
mount -o rw,remount /system
if [ ! -f "$TGT.orig" ]; then cp -a "$TGT" "$TGT.orig"; echo "backup created"; else echo "backup already exists"; fi
cat /data/local/tmp/libart-patched.so > "$TGT"
chmod 644 "$TGT"; chown root:root "$TGT"
chcon u:object_r:system_file:s0 "$TGT" 2>/dev/null || true
sync
echo "--- sizes ---"
ls -l "$TGT" "$TGT.orig"
echo "--- md5 (target vs what we pushed) ---"
md5sum "$TGT" /data/local/tmp/libart-patched.so
mount -o ro,remount /system
'@
Set-Content -Encoding ASCII F:\PN2Lineage\tools\_art_install.sh ($script -replace "`r`n","`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_art_install.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_art_install.sh'" 2>&1) | ForEach-Object { L "   $_" }

L ""
L "RECOVERY: adb shell su -c 'mount -o rw,remount /system; cp -a $TGT.orig $TGT; sync'"
L "or fastboot flash system F:\PN2Lineage\out\system-pn2-full.img"

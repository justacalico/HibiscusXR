# Install the 4 missing Pico libs + the merged public.libraries.txt, then retest.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\189_install.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== install missing Pico libs + public.libraries $(Get-Date) ==="

$libs = @('libvirtualinputclient.so','libairclient.so','libSafetyArea.so','libImageGrid.so')
foreach ($l in $libs) {
  foreach ($d in @('lib64','lib')) {
    $p = "F:\PN2Lineage\overlay_pvr\$d\$l"
    if (Test-Path $p) { & $adb -s $DEV push $p "/data/local/tmp/pvr_${d}_$l" 2>&1 | Out-Null }
  }
}
& $adb -s $DEV push F:\PN2Lineage\overlay_pvr\public.libraries.txt /data/local/tmp/public.libraries.txt 2>&1 | Out-Null

$sh = @'
set -e
mount -o rw,remount /system
for l in libvirtualinputclient.so libairclient.so libSafetyArea.so libImageGrid.so; do
  for d in lib64 lib; do
    s="/data/local/tmp/pvr_${d}_${l}"
    if [ -f "$s" ]; then
      cp "$s" "/system/$d/$l"
      chmod 644 "/system/$d/$l"; chown root:root "/system/$d/$l"
      chcon u:object_r:system_file:s0 "/system/$d/$l" 2>/dev/null || true
    fi
  done
done
cp /system/etc/public.libraries.txt /system/etc/public.libraries.txt.gsi 2>/dev/null || true
cp /data/local/tmp/public.libraries.txt /system/etc/public.libraries.txt
chmod 644 /system/etc/public.libraries.txt
sync
echo "--- installed ---"
ls -l /system/lib64/libvirtualinputclient.so /system/lib64/libairclient.so /system/lib64/libSafetyArea.so /system/lib64/libImageGrid.so
echo "--- public.libraries.txt tail ---"
tail -8 /system/etc/public.libraries.txt
'@
Set-Content -Encoding ASCII F:\PN2Lineage\tools\_pvrlibs.sh ($sh -replace "`r`n","`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_pvrlibs.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_pvrlibs.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting so the linker re-reads public.libraries.txt"
& $adb -s $DEV reboot 2>&1 | Out-Null

# Apply the merged public.libraries.txt now that BOTH the 64-bit and 32-bit
# dependency closures are verified for all 22 entries.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
& $adb -s $DEV push F:\PN2Lineage\overlay_pvr\public.libraries.txt /data/local/tmp/pl.txt 2>&1 | Out-Null
$sh = @(
  'mount -o rw,remount /system',
  'cp /data/local/tmp/pl.txt /system/etc/public.libraries.txt',
  'chmod 644 /system/etc/public.libraries.txt',
  'chown root:root /system/etc/public.libraries.txt',
  'sync',
  'echo "--- entries: $(grep -c "\.so$" /system/etc/public.libraries.txt) ---"',
  'tail -5 /system/etc/public.libraries.txt'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_wl.sh', $sh + "`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_wl.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_wl.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }
Write-Host "rebooting"
& $adb -s $DEV reboot 2>&1 | Out-Null

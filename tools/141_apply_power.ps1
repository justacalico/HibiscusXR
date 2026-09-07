# Wait for the port to come back, install the wakelock init rc, and take the
# wakelock immediately so it cannot suspend again before the next reboot.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$log = 'F:\PN2Lineage\notes\141_power.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== apply suspend fix $(Get-Date) ==="

for ($try = 1; $try -le 30; $try++) {
    & $adb -s $DEV wait-for-device 2>&1 | Out-Null
    $bc = ((& $adb -s $DEV shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "device up, boot_completed=$bc uptime=$(((& $adb -s $DEV shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim())s"

& $adb -s $DEV push F:\PN2Lineage\overlay\etc\init\pn2-power.rc /data/local/tmp/ 2>&1 | Out-Null

$sh = @'
#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp /data/local/tmp/pn2-power.rc /system/etc/init/pn2-power.rc
chmod 644 /system/etc/init/pn2-power.rc
chown root:root /system/etc/init/pn2-power.rc
sync
mount -o ro,remount /system
# apply now too, so it is protected before the next reboot
echo pn2_nosuspend > /sys/power/wake_lock
echo "rc installed: $(ls -l /system/etc/init/pn2-power.rc)"
echo "wakelocks now: $(cat /sys/power/wake_lock)"
'@
$f = 'F:\PN2Lineage\tools\_power_dev.sh'
[System.IO.File]::WriteAllText($f, ($sh -replace "`r`n","`n"))
& $adb -s $DEV push $f /data/local/tmp/power.sh 2>&1 | Out-Null
(& $adb -s $DEV shell "chmod 755 /data/local/tmp/power.sh; /system/xbin/su -c /data/local/tmp/power.sh" 2>&1) | ForEach-Object { L "   $_" }

# turn OFF the old stopgap so we are genuinely testing the wakelock
& $adb -s $DEV shell "svc power stayon false" 2>&1 | Out-Null
L "stayon turned OFF - the wakelock is now the only thing preventing suspend"
L "=== done ==="

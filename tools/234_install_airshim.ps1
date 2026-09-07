param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\234_airshim.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== airservice shim install $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

& $adb -s $Dev push F:\PN2Lineage\shim\libshim_air.so /data/local/tmp/libshim_air.so 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\overlay\etc\init\pn2-airservice.rc /data/local/tmp/pn2-airservice.rc 2>&1 | Out-Null

$sh = @(
  'mount -o rw,remount /system',
  'cp /data/local/tmp/libshim_air.so /system/lib64/pvr_air/libshim_air.so',
  'chmod 644 /system/lib64/pvr_air/libshim_air.so',
  'chown root:root /system/lib64/pvr_air/libshim_air.so',
  'chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libshim_air.so 2>/dev/null',
  'cp /data/local/tmp/pn2-airservice.rc /system/etc/init/pn2-airservice.rc',
  'chmod 644 /system/etc/init/pn2-airservice.rc',
  'sync',
  'ls /system/lib64/pvr_air/'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_airshim.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_airshim.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_airshim.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting (init must re-read the rc for the new setenv)"
& $adb -s $Dev shell "su -c 'sync; reboot'" 2>&1 | Out-Null

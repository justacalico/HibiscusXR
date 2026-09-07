# Install libdatabuffer.so (libairclient.so's missing dependency).
#
# Write the helper with WriteAllText + explicit \n: Set-Content appends CRLF to the
# final line, which is what turned earlier last-lines into "path\r" and made them
# fail with "No such file or directory".
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
foreach ($d in @('lib64','lib')) {
  & $adb -s $DEV push "F:\PN2Lineage\overlay_pvr\$d\libdatabuffer.so" "/data/local/tmp/db_$d.so" 2>&1 | Out-Null
}
$sh = @(
  'mount -o rw,remount /system',
  'cp /data/local/tmp/db_lib64.so /system/lib64/libdatabuffer.so',
  'cp /data/local/tmp/db_lib.so /system/lib/libdatabuffer.so',
  'chmod 644 /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so',
  'chown root:root /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so',
  'chcon u:object_r:system_file:s0 /system/lib64/libdatabuffer.so 2>/dev/null',
  'chcon u:object_r:system_file:s0 /system/lib/libdatabuffer.so 2>/dev/null',
  'sync',
  'ls -l /system/lib64/libdatabuffer.so /system/lib/libdatabuffer.so'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_db.sh', $sh + "`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_db.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_db.sh'" 2>&1) | ForEach-Object { Write-Host ("  " + $_) }

# Tombstones wrap by number, so pick by TIMESTAMP. Show everything from the
# post-ART-patch window (15:3x) with its process and top frames.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$names = (& $adb -s $DEV shell "su -c 'grep -l \"2026-08-13 15:3\" /data/tombstones/tombstone_*'" 2>&1) |
         ForEach-Object { "$_".Trim() } | Where-Object { $_ -match 'tombstone_\d+$' }
Write-Host ("post-patch tombstones: " + ($names -join ' '))
foreach ($n in $names) {
  Write-Host ""
  Write-Host ("########## " + $n)
  (& $adb -s $DEV shell "su -c 'cat $n'" 2>&1) |
    Where-Object { $_ -match 'Timestamp|^pid:|^signal|Cause|#0[0-8] pc' } |
    ForEach-Object { Write-Host ("  " + $_) }
}

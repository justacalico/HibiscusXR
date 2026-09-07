# Find the tombstone for the VRShell crash that now happens deep inside Unity
# (not the pvrservice SIGBUS, which was self-inflicted by overwriting a mapped .so).
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$hit = (& $adb -s $DEV shell "su -c 'grep -l com.pvr.vrshell /data/tombstones/tombstone_*'" 2>&1) |
       ForEach-Object { "$_".Trim() } | Where-Object { $_ -match 'tombstone_\d+$' }
Write-Host ("vrshell tombstones: " + ($hit -join ' '))
$newest = $hit | Select-Object -Last 1
Write-Host ("newest: " + $newest)
if ($newest) {
  (& $adb -s $DEV shell "su -c 'cat $newest'" 2>&1) | Select-Object -First 34 | ForEach-Object { Write-Host ("  " + $_) }
}

# Dump our device's real library inventory so dependency closure can be checked
# offline before anything goes into public.libraries.txt again.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$out = 'F:\PN2Lineage\notes\192_inventory.txt'
$dirs = @('/system/lib64','/vendor/lib64','/apex/com.android.runtime/lib64/bionic','/system/lib','/vendor/lib')
$all = @()
foreach ($d in $dirs) {
  $l = (& $adb -s $DEV shell "ls $d 2>/dev/null" 2>&1) | ForEach-Object { "$_".Trim() } | Where-Object { $_ -like '*.so' }
  Write-Host ("{0,-46} {1}" -f $d, $l.Count)
  $all += $l
}
Set-Content $out ($all | Sort-Object -Unique)
Write-Host ("unique libs on device: " + ($all | Sort-Object -Unique).Count + " -> $out")

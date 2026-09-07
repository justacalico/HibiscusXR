# Compare our Pico blob surface against the stock reference unit.
#
# Our VRShell2 launch logs a version mismatch that stock does not:
#   APP version, 2, 8, 6, 16   (what VRShell2 expects)
#   lib version, 2, 8, 6, 15   (what our /system ships)
#   open replaceable aborted
# followed by three config variables being undefined. That is the signature of a
# blob set assembled from a DIFFERENT firmware build than the apps. Hash both
# devices and list what differs.
$adb   = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
$out   = 'F:\PN2Lineage\notes\167_blobdiff.txt'
& $adb connect $STOCK 2>&1 | Out-Null

$cmd = "su -c 'md5sum /system/lib64/libPvr*.so /system/lib/libPvr*.so /system/etc/pvr/* /system/bin/pvrservice /system/lib64/libpvr*.so 2>/dev/null'"

function Grab($dev) {
  $h = @{}
  (& $adb -s $dev shell $cmd 2>&1) | ForEach-Object {
    $l = "$_".Trim()
    if ($l -match '^([0-9a-f]{32})\s+(\S+)$') { $h[$matches[2]] = $matches[1] }
  }
  return $h
}

$a = Grab $OURS
$b = Grab $STOCK
Set-Content $out "=== Pico blob comparison: ours vs stock (PUI 4.1.3) ==="
Add-Content $out ("ours : {0} files" -f $a.Count)
Add-Content $out ("stock: {0} files" -f $b.Count)
Add-Content $out ""

$all = ($a.Keys + $b.Keys) | Sort-Object -Unique
$same = 0; $diff = @(); $onlyOurs = @(); $onlyStock = @()
foreach ($k in $all) {
  if ($a.ContainsKey($k) -and $b.ContainsKey($k)) {
    if ($a[$k] -eq $b[$k]) { $same++ } else { $diff += $k }
  } elseif ($a.ContainsKey($k)) { $onlyOurs += $k } else { $onlyStock += $k }
}
Add-Content $out "identical : $same"
Add-Content $out ""
Add-Content $out "DIFFERENT CONTENT ($($diff.Count)):"
$diff | ForEach-Object { Add-Content $out ("  " + $_) }
Add-Content $out ""
Add-Content $out "MISSING FROM OURS ($($onlyStock.Count)):"
$onlyStock | ForEach-Object { Add-Content $out ("  " + $_) }
Add-Content $out ""
Add-Content $out "ONLY ON OURS ($($onlyOurs.Count)):"
$onlyOurs | ForEach-Object { Add-Content $out ("  " + $_) }

Add-Content $out ""
Add-Content $out "=== VRShell2 apk identical? ==="
foreach ($pair in @(@('ours',$OURS), @('stock',$STOCK))) {
  $m = (& $adb -s $pair[1] shell "su -c 'md5sum /system/priv-app/VRShell2/VRShell2.apk'" 2>&1)
  Add-Content $out ("  " + $pair[0] + ": " + (($m -join ' ').Trim()))
}
Get-Content $out

# Full /system/lib{,64} diff against the stock reference unit.
#
# Whitelisting libairclient.so without checking its dependencies bootlooped the
# device (zygote preloads every public library and aborts if one fails to link).
# Rather than chase one missing dependency at a time, enumerate EVERYTHING stock
# has that we do not - that is the real extent of the blob-set gap.
$adb   = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
$out   = 'F:\PN2Lineage\notes\191_libdiff.txt'
& $adb connect $STOCK 2>&1 | Out-Null

function LibList($dev, $dir) {
  (& $adb -s $dev shell "ls $dir 2>/dev/null" 2>&1) |
    ForEach-Object { "$_".Trim() } | Where-Object { $_ -like '*.so' }
}

Set-Content $out "=== /system/lib{,64}: stock vs ours ==="
foreach ($d in @('/system/lib64','/system/lib')) {
  $a = LibList $OURS  $d
  $b = LibList $STOCK $d
  $onlyStock = $b | Where-Object { $a -notcontains $_ }
  $onlyOurs  = $a | Where-Object { $b -notcontains $_ }
  Add-Content $out ""
  Add-Content $out "######## $d"
  Add-Content $out ("  ours: {0}   stock: {1}" -f $a.Count, $b.Count)
  Add-Content $out ""
  Add-Content $out ("  MISSING FROM OURS ({0}):" -f $onlyStock.Count)
  $onlyStock | Sort-Object | ForEach-Object { Add-Content $out ("    " + $_) }
  Add-Content $out ""
  Add-Content $out ("  only on ours ({0}) - GSI libs stock lacks, expected:" -f $onlyOurs.Count)
  $onlyOurs | Sort-Object | Select-Object -First 12 | ForEach-Object { Add-Content $out ("    " + $_) }
}
Get-Content $out

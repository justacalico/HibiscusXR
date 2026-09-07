# Diff /system/bin and /system/etc/init against stock.
#
# airservice (the passthrough camera service) was missing entirely - no binary, no
# init rc, no libairservice/libaircamera - which is why the see-through app blocks
# forever on "Waiting for service 'airservice'". Restoring the whitelist earlier
# only brought back the CLIENT library. Find every other daemon/init entry in the
# same situation before installing them one at a time.
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
$out   = 'F:\PN2Lineage\notes\224_bindiff.txt'

function Listing($dev, $cmd) {
  (& $adb -s $dev shell "su -c '$cmd'" 2>&1) | ForEach-Object { "$_".Trim() } | Where-Object { $_ -and $_ -notmatch '^/system/bin/sh' }
}

Set-Content $out "=== /system/bin and /system/etc/init: stock vs ours ==="
foreach ($d in @('/system/bin','/system/etc/init')) {
  $a = Listing $OURS  "ls $d"
  $b = Listing $STOCK "ls $d"
  $missing = $b | Where-Object { $a -notcontains $_ }
  Add-Content $out ""
  Add-Content $out "######## $d   (ours $($a.Count), stock $($b.Count))"
  Add-Content $out "  MISSING FROM OURS ($($missing.Count)):"
  $missing | Sort-Object | ForEach-Object { Add-Content $out "    $_" }
}
Get-Content $out

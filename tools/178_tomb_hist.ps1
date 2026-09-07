# Is the pvrservice serviceStatusCallback crash NEW (unmasked by the ART patch),
# or has it been happening all along behind the client's earlier death?
# Summarise every tombstone: time, process, signal, and top non-libc frame.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$out = 'F:\PN2Lineage\notes\178_tombs.txt'
Set-Content $out "=== tombstone history ==="

$names = (& $adb -s $DEV shell "su -c 'ls /data/tombstones/tombstone_* 2>/dev/null'" 2>&1) |
         ForEach-Object { "$_".Trim() } | Where-Object { $_ -match 'tombstone_\d+$' }

foreach ($n in $names) {
  $head = (& $adb -s $DEV shell "su -c 'grep -m1 Timestamp $n; grep -m1 \"^pid:\" $n; grep -m1 \"^signal\" $n; grep -m2 \"#0[0-9] pc\" $n'" 2>&1)
  $ts=''; $proc=''; $sig=''; $fr=@()
  foreach ($l in $head) {
    $s = "$l".Trim()
    if ($s -like 'Timestamp*') { $ts = $s -replace 'Timestamp:\s*','' }
    elseif ($s -like 'pid:*')  { $proc = ($s -split '>>>')[-1] -replace '<<<','' }
    elseif ($s -like 'signal*'){ $sig = ($s -split ',')[0] }
    elseif ($s -match '#0[0-9] pc') { $fr += ($s -replace '.*\s(/\S+)\s*\(?','$1 ') }
  }
  Add-Content $out ("{0,-22} {1,-28} {2}" -f $ts, $proc.Trim(), $sig)
  foreach ($f in $fr) { Add-Content $out ("      " + $f) }
}
Get-Content $out

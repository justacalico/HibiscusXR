# Full settings diff. The launcher decides to run Provision from somewhere; if it
# is a settings key that stock has and we do not, this finds it.
$adb = 'C:\adb\adb.exe'
$out = 'F:\PN2Lineage\notes\252_settings.txt'
Set-Content $out "=== settings: stock vs ours ==="
foreach ($ns in @('system','secure','global')) {
  $s = (& $adb -s 192.168.0.139:5555 shell "settings list $ns" 2>&1) | ForEach-Object { "$_".Trim() } | Where-Object { $_ -match '=' }
  $o = (& $adb -s 192.168.0.172:5555 shell "settings list $ns" 2>&1) | ForEach-Object { "$_".Trim() } | Where-Object { $_ -match '=' }
  $sk = @{}; foreach ($l in $s) { $k,$v = $l -split '=',2; $sk[$k] = $v }
  $ok = @{}; foreach ($l in $o) { $k,$v = $l -split '=',2; $ok[$k] = $v }

  Add-Content $out ""
  Add-Content $out "######## $ns"
  Add-Content $out "  -- keys ONLY on stock --"
  foreach ($k in ($sk.Keys | Sort-Object)) { if (-not $ok.ContainsKey($k)) { Add-Content $out ("    {0} = {1}" -f $k, $sk[$k]) } }
  Add-Content $out "  -- keys with DIFFERENT values --"
  foreach ($k in ($sk.Keys | Sort-Object)) {
    if ($ok.ContainsKey($k) -and $ok[$k] -ne $sk[$k]) {
      Add-Content $out ("    {0}: stock={1}  ours={2}" -f $k, $sk[$k], $ok[$k])
    }
  }
}
Get-Content $out

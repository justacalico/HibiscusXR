# Close the dependency set for airservice / virtual_input by pulling whatever they
# still need from stock, repeating until nothing is missing.
#
# Note libskia.so: Android 10 no longer ships it as a public /system library
# (Skia moved internal), so the 8.1 copy has to come along for libaircamera.so.
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\airsvc'
$NDK   = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$log   = 'F:\PN2Lineage\notes\226_close.log'
function L($m) { Add-Content $log $m; Write-Host $m }
Set-Content $log "=== closing dependency set ==="

$inv = @{}
(Get-Content 'F:\PN2Lineage\notes\192_inventory.txt') | ForEach-Object { $inv["$_".Trim()] = $true }

function PullLib($name) {
  $got = $false
  foreach ($d in @('lib64','lib')) {
    $dst = "$work\$d\$name"
    if (Test-Path $dst) { $got = $true; continue }
    & $adb -s $STOCK shell "su -c 'cp /system/$d/$name /data/local/tmp/_q 2>/dev/null; chmod 644 /data/local/tmp/_q'" 2>&1 | Out-Null
    & $adb -s $STOCK pull /data/local/tmp/_q $dst 2>&1 | Out-Null
    if (Test-Path $dst) { L ("   pulled {0,-24} {1,-6} {2} bytes" -f $name, $d, (Get-Item $dst).Length); $got = $true }
  }
  return $got
}

for ($round = 1; $round -le 5; $round++) {
  L ""
  L "--- round $round ---"
  $missing = @()
  foreach ($f in (Get-ChildItem "$work\bin","$work\lib64","$work\lib" -File -EA SilentlyContinue)) {
    $needed = & $NDK -dW $f.FullName 2>&1 | Select-String 'NEEDED' | ForEach-Object {
        if ($_.Line -match '\[([^\]]+)\]') { $Matches[1] } }
    foreach ($n in $needed) {
      if ($inv.ContainsKey($n)) { continue }
      if ((Test-Path "$work\lib64\$n") -or (Test-Path "$work\lib\$n")) { continue }
      $missing += $n
    }
  }
  $missing = $missing | Sort-Object -Unique
  if (-not $missing) { L "   nothing missing - dependency set is closed"; break }
  L ("   still missing: " + ($missing -join ', '))
  foreach ($m in $missing) { if (-not (PullLib $m)) { L "   COULD NOT PULL $m" } }
}

L ""
L "=== final staged set ==="
Get-ChildItem "$work\bin","$work\lib64","$work\lib","$work\init" -File -EA SilentlyContinue |
  ForEach-Object { L ("   {0,-46} {1,10}" -f $_.FullName.Replace("$work\",''), $_.Length) }

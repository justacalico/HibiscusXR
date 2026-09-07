# Per-binary dependency tree, so we install the minimum and avoid dragging 8.1's
# libskia + ICU into a Q system (ICU lives in an APEX on Q; shadowing it in
# /system/lib64 could break the framework everywhere).
$work = 'F:\PN2Lineage\airsvc'
$NDK  = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$inv  = @{}
(Get-Content 'F:\PN2Lineage\notes\192_inventory.txt') | ForEach-Object { $inv["$_".Trim()] = $true }

function Needed($path) {
  & $NDK -dW $path 2>&1 | Select-String 'NEEDED' | ForEach-Object {
      if ($_.Line -match '\[([^\]]+)\]') { $Matches[1] } }
}

function Tree($name, $depth, $seen) {
  $pad = '  ' * $depth
  $local = @("$work\lib64\$name", "$work\lib\$name") | Where-Object { Test-Path $_ } | Select-Object -First 1
  $where = if ($inv.ContainsKey($name)) { 'on device' } elseif ($local) { 'STAGED (8.1)' } else { 'MISSING' }
  Write-Host ("  {0}{1,-26} {2}" -f $pad, $name, $where)
  if (-not $local -or $seen.Contains($name) -or $depth -ge 3) { return }
  $seen.Add($name) | Out-Null
  foreach ($n in (Needed $local)) {
    if ($inv.ContainsKey($n)) { continue }   # already satisfied by the device
    Tree $n ($depth + 1) $seen
  }
}

foreach ($b in @('airservice','virtual_input')) {
  Write-Host ""
  Write-Host "######## $b"
  $p = "$work\bin\$b"
  $seen = New-Object System.Collections.Generic.HashSet[string]
  foreach ($n in (Needed $p)) {
    if ($inv.ContainsKey($n)) { Write-Host ("    {0,-26} on device" -f $n); continue }
    Tree $n 1 $seen
  }
}

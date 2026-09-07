# For each missing symbol, what does Q actually export nearby? That decides
# whether a shim is a one-line forward or a reimplementation of libgui internals.
$NDK  = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$qlib = 'F:\PN2Lineage\notes\qlibs'

$all = @()
foreach ($f in (Get-ChildItem $qlib -Filter *.so)) {
  & $NDK --dyn-syms -W $f.FullName 2>&1 | ForEach-Object {
    if ($_ -notmatch '\sUND\s' -and $_ -match '\s(\S+)$') { $all += $Matches[1] }
  }
}
$all = $all | Sort-Object -Unique
Write-Host ("indexed " + $all.Count + " exported symbols")

$patterns = @{
  'Fence dtor'            = '_ZN7android5FenceD'
  'CpuConsumer lock'      = 'lockImageFromBuffer|getLockedImageInfo|CpuConsumer'
  'OutputConfiguration'   = 'OutputConfigurationC[12]'
  'BufferItemConsumer'    = 'BufferItemConsumer'
  'GraphicBuffer lock'    = '_ZN7android13GraphicBuffer4lock'
}
foreach ($k in $patterns.Keys) {
  Write-Host ""
  Write-Host "######## $k"
  $hits = $all | Where-Object { $_ -match $patterns[$k] } | Select-Object -First 12
  if ($hits) { $hits | ForEach-Object { Write-Host ("    " + $_) } } else { Write-Host "    (nothing)" }
}

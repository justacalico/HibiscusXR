# What does libaircamera actually use libskia for?
#
# libskia (8.1) drags ICU 60, which conflicts irreconcilably with Q's ICU 63 in the
# same process. If libaircamera only touches a handful of skia entry points (say an
# image encoder for debug frames), a small stub libskia can replace it and the whole
# ICU problem disappears.
$NDK  = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$work = 'F:\PN2Lineage\airsvc'

# everything libskia exports
$skia = @{}
& $NDK --dyn-syms -W "$work\lib64\libskia.so" 2>&1 | ForEach-Object {
  if ($_ -notmatch '\sUND\s' -and $_ -match '\s(\S+)$') { $skia[$Matches[1]] = $true }
}
Write-Host ("libskia exports: " + $skia.Count)

foreach ($lib in @('libaircamera.so','libairservice.so')) {
  $need = @()
  & $NDK --dyn-syms -W "$work\lib64\$lib" 2>&1 | ForEach-Object {
    if ($_ -match '\sUND\s+(\S+)$') { $n = $Matches[1]; if ($skia.ContainsKey($n)) { $need += $n } }
  }
  $need = $need | Sort-Object -Unique
  Write-Host ""
  Write-Host "######## $lib needs $($need.Count) symbols from libskia"
  $need | ForEach-Object { Write-Host ("    " + $_) }
}

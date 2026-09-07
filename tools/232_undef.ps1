# Every undefined symbol in the staged 8.1 libs that Q's system libraries do not
# provide. Finding them all at once beats rediscovering them one linker error per
# reboot.
$adb  = 'C:\adb\adb.exe'
$DEV  = '192.168.0.172:5555'
$NDK  = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$work = 'F:\PN2Lineage\airsvc'
$qlib = 'F:\PN2Lineage\notes\qlibs'
New-Item -ItemType Directory -Force $qlib | Out-Null

# pull the Q system libs the staged ones link against
$want = @('libgui.so','libui.so','libcamera_client.so','libbinder.so','libutils.so','libsensor.so','libcamera_metadata.so','libcutils.so','libdatabuffer.so')
foreach ($l in $want) {
  if (Test-Path "$qlib\$l") { continue }
  & $adb -s $DEV shell "su -c 'cp /system/lib64/$l /data/local/tmp/_r 2>/dev/null; chmod 644 /data/local/tmp/_r'" 2>&1 | Out-Null
  & $adb -s $DEV pull /data/local/tmp/_r "$qlib\$l" 2>&1 | Out-Null
}
Write-Host ("Q libs staged: " + (Get-ChildItem $qlib -Filter *.so).Count)

# build the set of everything Q exports
$exports = @{}
foreach ($f in (Get-ChildItem $qlib -Filter *.so)) {
  & $NDK --dyn-syms -W $f.FullName 2>&1 | ForEach-Object {
    if ($_ -match '\s(FUNC|OBJECT)\s+\S+\s+\S+\s+(\d+|\S+)\s+(\S+)$') {
      $n = $Matches[3]
      if ($_ -notmatch '\sUND\s') { $exports[$n] = $true }
    }
  }
}
Write-Host ("Q exported symbols indexed: " + $exports.Count)
Write-Host ""

foreach ($lib in @('libaircamera.so','libairservice.so','libvirtualinput.so')) {
  $p = "$work\lib64\$lib"
  if (-not (Test-Path $p)) { continue }
  $missing = @()
  & $NDK --dyn-syms -W $p 2>&1 | ForEach-Object {
    if ($_ -match '\sUND\s+(\S+)$') {
      $n = $Matches[1]
      if (-not $exports.ContainsKey($n)) { $missing += $n }
    }
  }
  # ignore libc/libc++/libm/libdl names - those resolve from bionic, not the libs we indexed
  $missing = $missing | Where-Object { $_ -match '^_ZN7android|^_ZNK7android' } | Sort-Object -Unique
  Write-Host "######## $lib : $($missing.Count) android:: symbols not found in Q"
  $missing | ForEach-Object { Write-Host ("    " + $_) }
}

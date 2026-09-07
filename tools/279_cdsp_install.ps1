# Install the DSP RPC libraries into /system so qvrservice can load them.
#
# qvrservice runs from /system/bin, so it uses the system linker namespace and
# cannot reach /vendor/lib. Qualcomm ships *_system.so copies for this; stock has
# them, we do not, so libqvr_cdsp_driver_stub.so fails to dlopen libcdsprpc.so,
# the DSP wrapper dies "UNRECOVERABLE", the tracker never initialises and QVR
# reports "VR mode is not supported" - which zeroes trackingstate and blacks out
# the display.
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\cdsp'
$NDK   = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$log   = 'F:\PN2Lineage\notes\279_cdsp.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== dsp rpc libs $(Get-Date) ==="
New-Item -ItemType Directory -Force "$work\lib","$work\lib64" | Out-Null
& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $OURS  2>&1 | Out-Null

$libs = @('libcdsprpc_system.so','libadsprpc_system.so','libsdsprpc_system.so')
foreach ($l in $libs) {
  foreach ($d in @('lib','lib64')) {
    $dst = "$work\$d\$l"
    if (Test-Path $dst) { continue }
    & $adb -s $STOCK shell "su -c 'cp /system/$d/$l /data/local/tmp/_c 2>/dev/null; chmod 644 /data/local/tmp/_c'" 2>&1 | Out-Null
    & $adb -s $STOCK pull /data/local/tmp/_c $dst 2>&1 | Out-Null
    if (Test-Path $dst) { L ("pulled {0,-26} {1,-6} {2} bytes" -f $l, $d, (Get-Item $dst).Length) }
  }
}

L ""
L "--- SONAME of each (this decides what filename the loader will match) ---"
foreach ($f in (Get-ChildItem "$work\lib64" -Filter *.so)) {
  $so = & $NDK -dW $f.FullName 2>&1 | Select-String 'SONAME' | ForEach-Object { ($_ -replace '.*\[([^\]]+)\].*','$1') }
  L ("   {0,-26} SONAME = {1}" -f $f.Name, $so)
}
L ""
L "--- their dependencies ---"
foreach ($f in (Get-ChildItem "$work\lib64" -Filter *.so)) {
  $needed = & $NDK -dW $f.FullName 2>&1 | Select-String 'NEEDED' | ForEach-Object { ($_ -replace '.*\[([^\]]+)\].*','$1') }
  L ("   {0}: {1}" -f $f.Name, ($needed -join ', '))
}

# Restore the airservice (passthrough camera) and virtual_input daemons.
#
# The see-through app blocks forever on "Waiting for service 'airservice'".
# Stock runs /system/bin/airservice as pvr.IAIRService from an init rc we never
# installed. libaircamera.so names what it is: the passthrough camera path.
#
# virtual_input goes with it - I restored libvirtualinputclient.so earlier (it
# defines pvrVirtualInputCreate) but not the daemon behind it.
#
# Dependency-check every library before installing, the same discipline the
# public.libraries bootloop taught.
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\airsvc'
$log   = 'F:\PN2Lineage\notes\225_airservice.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== airservice + virtual_input restore $(Get-Date) ==="
New-Item -ItemType Directory -Force "$work\bin","$work\lib64","$work\lib","$work\init" | Out-Null

& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $OURS  2>&1 | Out-Null

function PullFrom($remote, $local) {
  & $adb -s $STOCK shell "su -c 'cp $remote /data/local/tmp/_p 2>/dev/null; chmod 644 /data/local/tmp/_p'" 2>&1 | Out-Null
  & $adb -s $STOCK pull /data/local/tmp/_p $local 2>&1 | Out-Null
  return (Test-Path $local)
}

L "--- pulling binaries ---"
foreach ($b in @('airservice','virtual_input','airclient_test')) {
  if (PullFrom "/system/bin/$b" "$work\bin\$b") { L ("   {0,-18} {1} bytes" -f $b, (Get-Item "$work\bin\$b").Length) }
  else { L "   $b  NOT FOUND" }
}

L "--- pulling init rc ---"
foreach ($r in @('airservice.rc','virtual_input.rc')) {
  if (PullFrom "/system/etc/init/$r" "$work\init\$r") { L ("   {0,-18} {1} bytes" -f $r, (Get-Item "$work\init\$r").Length) }
}

L "--- pulling libs (both ABIs) ---"
foreach ($l in @('libairservice.so','libaircamera.so')) {
  foreach ($d in @('lib64','lib')) {
    if (PullFrom "/system/$d/$l" "$work\$d\$l") { L ("   {0,-20} {1,-6} {2} bytes" -f $l, $d, (Get-Item "$work\$d\$l").Length) }
  }
}

L ""
L "--- what do these need? (checked against our inventory before install) ---"
$NDK = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$inv = @{}
(Get-Content 'F:\PN2Lineage\notes\192_inventory.txt') | ForEach-Object { $inv["$_".Trim()] = $true }
$missing = @()
foreach ($f in (Get-ChildItem "$work\bin","$work\lib64","$work\lib" -File)) {
  $needed = & $NDK -dW $f.FullName 2>&1 | Select-String 'NEEDED' | ForEach-Object {
      if ($_.Line -match '\[([^\]]+)\]') { $Matches[1] } }
  foreach ($n in $needed) {
    if (-not $inv.ContainsKey($n)) {
      # might be one of the ones we are installing right now
      if (-not (Test-Path "$work\lib64\$n") -and -not (Test-Path "$work\lib\$n")) {
        $missing += "$($f.Name) -> $n"
      }
    }
  }
}
if ($missing) { $missing | Sort-Object -Unique | ForEach-Object { L ("   MISSING: " + $_) } }
else { L "   all dependencies satisfied" }

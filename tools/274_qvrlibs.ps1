# qvrservice loads none of its plugins ("Plugin not valid") and so returns a NULL
# client, which zeroes trackingstate and kills ALL tracking, not just 6DoF.
# The QVR driver stubs are present; these three are not. Install and retest.
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\qvrlibs'
$log   = 'F:\PN2Lineage\notes\274_qvrlibs.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== qvr plugin libs $(Get-Date) ==="
New-Item -ItemType Directory -Force "$work\lib","$work\lib64" | Out-Null
& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $OURS  2>&1 | Out-Null

$libs = @('libtobii_runtime.so','libtobii_eyecore_stub.so','libqti-perfd-client_system.so')
foreach ($l in $libs) {
  foreach ($d in @('lib','lib64')) {
    $dst = "$work\$d\$l"
    if (Test-Path $dst) { continue }
    & $adb -s $STOCK shell "su -c 'cp /system/$d/$l /data/local/tmp/_ql 2>/dev/null; chmod 644 /data/local/tmp/_ql'" 2>&1 | Out-Null
    & $adb -s $STOCK pull /data/local/tmp/_ql $dst 2>&1 | Out-Null
    if (Test-Path $dst) { L ("pulled {0,-32} {1,-6} {2} bytes" -f $l, $d, (Get-Item $dst).Length) }
  }
}

# check their deps against our inventory before installing
$NDK = 'F:\Android\Sdk\ndk\23.1.7779620\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe'
$inv = @{}
(Get-Content 'F:\PN2Lineage\notes\192_inventory.txt') | ForEach-Object { $inv["$_".Trim()] = $true }
L ""
L "--- dependency check ---"
$bad = 0
foreach ($f in (Get-ChildItem "$work\lib","$work\lib64" -Filter *.so -EA SilentlyContinue)) {
  $needed = & $NDK -dW $f.FullName 2>&1 | Select-String 'NEEDED' | ForEach-Object { ($_ -replace '.*\[([^\]]+)\].*','$1') }
  foreach ($n in $needed) {
    if (-not $inv.ContainsKey($n) -and -not (Test-Path "$work\lib\$n") -and -not (Test-Path "$work\lib64\$n")) {
      L ("   MISSING DEP: {0} -> {1}" -f $f.Name, $n); $bad++
    }
  }
}
if ($bad -eq 0) { L "   all dependencies satisfied" }

foreach ($f in (Get-ChildItem "$work\lib","$work\lib64" -Filter *.so -EA SilentlyContinue)) {
  $abi = Split-Path (Split-Path $f.FullName -Parent) -Leaf
  & $adb -s $OURS push $f.FullName "/data/local/tmp/qvr_${abi}_$($f.Name)" 2>&1 | Out-Null
}

$sh = @(
  'mount -o rw,remount /system',
  'for f in /data/local/tmp/qvr_lib_*.so;   do b=$(basename $f); b=${b#qvr_lib_};   cp "$f" "/system/lib/$b";   chmod 644 "/system/lib/$b"; done',
  'for f in /data/local/tmp/qvr_lib64_*.so; do b=$(basename $f); b=${b#qvr_lib64_}; cp "$f" "/system/lib64/$b"; chmod 644 "/system/lib64/$b"; done',
  'chown root:root /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null',
  'chcon u:object_r:system_lib_file:s0 /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null',
  'sync',
  'ls -l /system/lib/libtobii*.so /system/lib/libqti-perfd*.so 2>/dev/null',
  'echo "--- restarting qvrd ---"',
  'stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 6',
  'getprop init.svc.pn2_qvrd',
  'ps -A | grep -i qvrservice',
  'echo "--- rss (stock is ~7.4MB with plugins) ---"',
  'grep VmRSS /proc/$(pidof qvrservice)/status',
  'echo "--- plugins mapped now? ---"',
  'grep -oE "/system/lib/libqvr[^ ]*\.so|/system/lib/libtobii[^ ]*\.so" /proc/$(pidof qvrservice)/maps | sort -u'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_qvrlibs.sh', $sh + "`n")
& $adb -s $OURS push F:\PN2Lineage\tools\_qvrlibs.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $OURS shell "su -c 'sh /data/local/tmp/_qvrlibs.sh'" 2>&1) | ForEach-Object { L "   $_" }

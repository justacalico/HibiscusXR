# Install the DSP RPC libs into /system under BOTH names.
#
# libqvr_cdsp_driver_stub.so does dlopen("libcdsprpc.so"), which matches on
# FILENAME, not SONAME - so a copy named libcdsprpc.so in /system/lib works even
# though its SONAME says _system. Android 8.1 let /system processes reach
# /vendor/lib; Android 10's namespace separation does not, which is why this
# worked on stock and not here.
#
# Their deps are all plain system libs (liblog/libcutils/libc++), so nothing
# vendor-side comes along.
param([string]$Dev = '192.168.0.172:5555')
$adb  = 'C:\adb\adb.exe'
$work = 'F:\PN2Lineage\cdsp'
$log  = 'F:\PN2Lineage\notes\280_cdsp_apply.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== install dsp rpc libs $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

foreach ($d in @('lib','lib64')) {
  foreach ($f in (Get-ChildItem "$work\$d" -Filter *.so -EA SilentlyContinue)) {
    & $adb -s $Dev push $f.FullName "/data/local/tmp/dsp_${d}_$($f.Name)" 2>&1 | Out-Null
  }
}

$sh = @(
  'mount -o rw,remount /system',
  'for f in /data/local/tmp/dsp_lib_*.so; do',
  '  b=$(basename $f); b=${b#dsp_lib_}',
  '  cp "$f" "/system/lib/$b"',
  '  # also under the bare name the qvr stub actually dlopens',
  '  bare=$(echo "$b" | sed "s/_system//")',
  '  cp "$f" "/system/lib/$bare"',
  'done',
  'for f in /data/local/tmp/dsp_lib64_*.so; do',
  '  b=$(basename $f); b=${b#dsp_lib64_}',
  '  cp "$f" "/system/lib64/$b"',
  '  bare=$(echo "$b" | sed "s/_system//")',
  '  cp "$f" "/system/lib64/$bare"',
  'done',
  'chmod 644 /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so',
  'chown root:root /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so',
  'chcon u:object_r:system_lib_file:s0 /system/lib/lib?dsprpc*.so /system/lib64/lib?dsprpc*.so 2>/dev/null',
  'sync',
  'echo "--- installed ---"',
  'ls -l /system/lib/lib?dsprpc*.so',
  'echo "--- restarting qvrd ---"',
  'stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8',
  'echo "--- rss (stock ~7.4MB when the tracker is up) ---"',
  'grep VmRSS /proc/$(pidof qvrservice)/status',
  'echo "--- QVR now ---"',
  'logcat -d -t 200 | grep -iE "QVRService|DspWrapper|Tracker" | tail -12'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_dsp.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_dsp.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_dsp.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

# Install /system/lib/rfsa/adsp - the DSP-side QVR skel libraries.
#
# FastRPC's remote_handle_open loads a library ONTO the Compute DSP; it finds it
# on the normal filesystem under lib/rfsa. Stock has libqvr_dsp_driver_skel.so
# there and we have no rfsa directory at all, so the handle is always null:
#
#   QVRServiceDspWrapper: Failed to open QVR DSP Driver skel: remote handle is null
#   -> tracker init failed -> "VR mode is not supported" -> trackingstate 0
#   -> pose rejected -> black screen
$adb   = 'C:\adb\adb.exe'
$OURS  = '192.168.0.172:5555'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\rfsa'
$log   = 'F:\PN2Lineage\notes\293_rfsa.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== rfsa dsp skels $(Get-Date) ==="
New-Item -ItemType Directory -Force $work | Out-Null
& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $OURS  2>&1 | Out-Null

L "--- everything in stock's /system/lib/rfsa/adsp ---"
$names = (& $adb -s $STOCK shell "su -c 'ls /system/lib/rfsa/adsp/'" 2>&1) |
         ForEach-Object { "$_".Trim() } | Where-Object { $_ -like '*.so' }
foreach ($n in $names) {
  & $adb -s $STOCK shell "su -c 'cp /system/lib/rfsa/adsp/$n /data/local/tmp/_r; chmod 644 /data/local/tmp/_r'" 2>&1 | Out-Null
  & $adb -s $STOCK pull /data/local/tmp/_r "$work\$n" 2>&1 | Out-Null
  if (Test-Path "$work\$n") { L ("   {0,-42} {1,10:N0}" -f $n, (Get-Item "$work\$n").Length) }
}
L ("pulled {0} skel libraries" -f (Get-ChildItem $work -Filter *.so).Count)

foreach ($f in (Get-ChildItem $work -Filter *.so)) {
  & $adb -s $OURS push $f.FullName "/data/local/tmp/rfsa_$($f.Name)" 2>&1 | Out-Null
}

$sh = @(
  'mount -o rw,remount /system',
  'mkdir -p /system/lib/rfsa/adsp',
  'for f in /data/local/tmp/rfsa_*.so; do',
  '  b=$(basename $f); b=${b#rfsa_}',
  '  cp "$f" "/system/lib/rfsa/adsp/$b"',
  'done',
  'chmod 755 /system/lib/rfsa /system/lib/rfsa/adsp',
  'chmod 644 /system/lib/rfsa/adsp/*.so',
  'chown -R root:root /system/lib/rfsa',
  'chcon -R u:object_r:system_file:s0 /system/lib/rfsa 2>/dev/null',
  'sync',
  'echo "--- installed ---"',
  'ls /system/lib/rfsa/adsp/',
  'echo "--- restart qvrd ---"',
  'stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8',
  'grep VmRSS /proc/$(pidof qvrservice)/status',
  'echo "--- THE TEST ---"',
  'logcat -c',
  'timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -10',
  'sleep 1',
  'logcat -d | grep -iE "DspWrapper|QVRServiceTracker|VR mode|Plugin" | tail -8',
  'rm -f /data/local/tmp/rfsa_*.so'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_rfsa.sh', $sh + "`n")
& $adb -s $OURS push F:\PN2Lineage\tools\_rfsa.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $OURS shell "su -c 'sh /data/local/tmp/_rfsa.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

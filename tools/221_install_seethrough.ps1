# Install the signed see-through calibration app into /system/priv-app.
param([string]$Dev = '192.168.0.172:5555')
$adb  = 'C:\adb\adb.exe'
$work = 'F:\PN2Lineage\seethrough'
$log  = 'F:\PN2Lineage\notes\221_install_st.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== install seethroughsetting $(Get-Date) ==="

& $adb connect $Dev 2>&1 | Out-Null
L "--- free space on /system before ---"
(& $adb -s $Dev shell "df -h /system" 2>&1) | ForEach-Object { L ("   " + $_) }

L "pushing signed apk (194 MB)"
& $adb -s $Dev push "$work\seethroughsetting-signed.apk" /data/local/tmp/seethroughsetting.apk 2>&1 |
    Select-Object -Last 1 | ForEach-Object { L "   $_" }

foreach ($l in (Get-ChildItem "$work\lib\arm64" -Filter *.so)) {
    & $adb -s $Dev push $l.FullName "/data/local/tmp/st_$($l.Name)" 2>&1 | Out-Null
}
L ("pushed " + (Get-ChildItem "$work\lib\arm64" -Filter *.so).Count + " native libs")

$sh = @(
  'D=/system/priv-app/seethroughsetting',
  'mount -o rw,remount /system',
  'mkdir -p $D/lib/arm64',
  'cp /data/local/tmp/seethroughsetting.apk $D/seethroughsetting.apk',
  'for f in /data/local/tmp/st_*.so; do',
  '  b=$(basename $f); b=${b#st_}',
  '  cp "$f" "$D/lib/arm64/$b"',
  'done',
  'chmod 755 $D $D/lib $D/lib/arm64',
  'chmod 644 $D/seethroughsetting.apk $D/lib/arm64/*.so',
  'chown -R root:root $D',
  'chcon -R u:object_r:system_file:s0 $D 2>/dev/null',
  'sync',
  'echo "--- installed ---"',
  'ls -l $D',
  'ls $D/lib/arm64 | wc -l',
  'df -h /system',
  'rm -f /data/local/tmp/st_*.so /data/local/tmp/seethroughsetting.apk'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_st.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_st.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_st.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting so PackageManager scans it"
& $adb -s $Dev shell "su -c 'sync; reboot'" 2>&1 | Out-Null

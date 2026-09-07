# Install airservice + virtual_input, with the 8.1 skia/ICU chain isolated.
param([string]$Dev = '192.168.0.172:5555')
$adb  = 'C:\adb\adb.exe'
$work = 'F:\PN2Lineage\airsvc'
$log  = 'F:\PN2Lineage\notes\228_install_air.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== airservice install $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

# private chain: only airservice sees these
$private = @('libairservice.so','libaircamera.so','libskia.so','libicuuc.so','libicui18n.so')
# safe to install normally: no Q counterpart to shadow
$shared  = @('libvirtualinput.so')

foreach ($f in $private) { & $adb -s $Dev push "$work\lib64\$f" "/data/local/tmp/air64_$f" 2>&1 | Out-Null }
foreach ($f in $shared)  { & $adb -s $Dev push "$work\lib64\$f" "/data/local/tmp/sh64_$f" 2>&1 | Out-Null }
foreach ($b in @('airservice','virtual_input')) { & $adb -s $Dev push "$work\bin\$b" "/data/local/tmp/bin_$b" 2>&1 | Out-Null }
& $adb -s $Dev push F:\PN2Lineage\overlay\etc\init\pn2-airservice.rc /data/local/tmp/pn2-airservice.rc 2>&1 | Out-Null
L "staged files pushed"

$sh = @(
  'mount -o rw,remount /system',
  'mkdir -p /system/lib64/pvr_air',
  'for f in /data/local/tmp/air64_*.so; do b=$(basename $f); b=${b#air64_}; cp "$f" "/system/lib64/pvr_air/$b"; done',
  'for f in /data/local/tmp/sh64_*.so;  do b=$(basename $f); b=${b#sh64_};  cp "$f" "/system/lib64/$b"; done',
  'chmod 755 /system/lib64/pvr_air',
  'chmod 644 /system/lib64/pvr_air/*.so /system/lib64/libvirtualinput.so',
  'chown -R root:root /system/lib64/pvr_air',
  'chcon -R u:object_r:system_lib_file:s0 /system/lib64/pvr_air 2>/dev/null || chcon -R u:object_r:system_file:s0 /system/lib64/pvr_air 2>/dev/null',
  'cp /data/local/tmp/bin_airservice /system/bin/airservice',
  'cp /data/local/tmp/bin_virtual_input /system/bin/virtual_input',
  'chmod 755 /system/bin/airservice /system/bin/virtual_input',
  'chown root:root /system/bin/airservice /system/bin/virtual_input',
  'chcon u:object_r:system_file:s0 /system/bin/airservice /system/bin/virtual_input 2>/dev/null',
  'cp /data/local/tmp/pn2-airservice.rc /system/etc/init/pn2-airservice.rc',
  'chmod 644 /system/etc/init/pn2-airservice.rc',
  'sync',
  'echo "--- installed ---"',
  'ls -l /system/bin/airservice /system/bin/virtual_input',
  'ls /system/lib64/pvr_air/',
  'ls -l /system/lib64/libvirtualinput.so',
  'rm -f /data/local/tmp/air64_*.so /data/local/tmp/sh64_*.so /data/local/tmp/bin_*'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_air.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_air.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_air.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting so init picks up the new services"
& $adb -s $Dev shell "su -c 'sync; reboot'" 2>&1 | Out-Null

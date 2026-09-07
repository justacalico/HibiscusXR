# airservice links against 8.1's tinyxml2: it wants
#   tinyxml2::XMLDocument::XMLDocument(bool)
# but Q's libtinyxml2.so only offers XMLDocument(bool, Whitespace). The library is
# present on our device, which is why the DT_NEEDED scan passed - the break is in
# the symbol, not the file.
#
# Drop the 8.1 copy into the private pvr_air dir so only airservice sees it.
param([string]$Dev = '192.168.0.172:5555')
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$work  = 'F:\PN2Lineage\airsvc'
$log   = 'F:\PN2Lineage\notes\230_tinyxml.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== tinyxml2 for airservice $(Get-Date) ==="
& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $Dev   2>&1 | Out-Null

& $adb -s $STOCK shell "su -c 'cp /system/lib64/libtinyxml2.so /data/local/tmp/_t 2>/dev/null; chmod 644 /data/local/tmp/_t'" 2>&1 | Out-Null
& $adb -s $STOCK pull /data/local/tmp/_t "$work\lib64\libtinyxml2.so" 2>&1 | Out-Null
if (Test-Path "$work\lib64\libtinyxml2.so") { L ("pulled 8.1 libtinyxml2.so: " + (Get-Item "$work\lib64\libtinyxml2.so").Length + " bytes") }
else { L "could not pull libtinyxml2.so"; exit 1 }

& $adb -s $Dev push "$work\lib64\libtinyxml2.so" /data/local/tmp/txml.so 2>&1 | Out-Null
$sh = @(
  'mount -o rw,remount /system',
  'cp /data/local/tmp/txml.so /system/lib64/pvr_air/libtinyxml2.so',
  'chmod 644 /system/lib64/pvr_air/libtinyxml2.so',
  'chown root:root /system/lib64/pvr_air/libtinyxml2.so',
  'chcon u:object_r:system_lib_file:s0 /system/lib64/pvr_air/libtinyxml2.so 2>/dev/null',
  'sync',
  'ls /system/lib64/pvr_air/',
  'echo "--- restarting airservice ---"',
  'stop airservice',
  'start airservice',
  'sleep 4',
  'getprop init.svc.airservice',
  'service list 2>/dev/null | grep -i air',
  'ps -A 2>/dev/null | grep -i airservice'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_txml.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_txml.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_txml.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "--- any remaining link errors ---"
(& $adb -s $Dev shell "logcat -d -t 200 2>/dev/null | grep -i 'CANNOT LINK' | tail -4" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

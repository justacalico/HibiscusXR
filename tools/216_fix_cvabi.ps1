# Correct CVService's cached ABI.
#
# PackageManager derives primaryCpuAbi when it first scans a package and caches it
# in /data/system/packages.xml. CVService was first scanned before the app-private
# lib/arm directory was restored, so with no native libs visible PM defaulted to
# arm64-v8a and kept it. The libs are 32-bit, so the service could never load and
# it ended up disabled.
#
# Stock records armeabi-v7a. Rewrite the cached value to match and re-enable.
# The framework is stopped first so system_server cannot rewrite packages.xml
# underneath the edit.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\216_cvabi.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== CVService ABI fix $(Get-Date) ==="

$sh = @(
  'P=/data/system/packages.xml',
  'echo "--- before ---"',
  'grep -o "name=\"com.picovr.picovrlib.cvcontroller\"[^>]*" $P | head -1',
  'grep -A2 "com.picovr.picovrlib.cvcontroller" $P | grep -o "primaryCpuAbi=\"[^\"]*\"" | head -1',
  'cp $P $P.bak',
  '# stop the framework so nothing rewrites packages.xml under us',
  'stop',
  'sleep 3',
  '# only touch the cvcontroller package block',
  'sed -i "/com.picovr.picovrlib.cvcontroller/,/<\/package>/ s/primaryCpuAbi=\"arm64-v8a\"/primaryCpuAbi=\"armeabi-v7a\"/" $P',
  'sync',
  'echo "--- after ---"',
  'grep -A4 "com.picovr.picovrlib.cvcontroller" $P | grep -o "primaryCpuAbi=\"[^\"]*\"" | head -1'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_cvabi.sh', $sh + "`n")
& $adb connect $Dev 2>&1 | Out-Null
& $adb -s $Dev push F:\PN2Lineage\tools\_cvabi.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_cvabi.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "rebooting"
& $adb -s $Dev shell "su -c 'sync; reboot'" 2>&1 | Out-Null

# Use the /vendor copies of the DSP RPC libs, not the _system ones.
#
# The stub's verneed asks for File: libcdsprpc.so, Version: SDSPRPC. The /vendor
# copy declares SONAME libcdsprpc.so and defines SDSPRPC, so it matches exactly.
# The _system copy declares SONAME libcdsprpc_system.so, which is why the loader
# rejected it on symbol versioning even after renaming the file.
#
# Its DT_NEEDED is liblog/libcutils/libc++/libc/libm/libdl - all plain system
# libraries, so nothing vendor-side is dragged into the system namespace.
param([string]$Dev = '192.168.0.172:5555')
$adb = 'C:\adb\adb.exe'
$log = 'F:\PN2Lineage\notes\283_vendor_cdsp.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== vendor dsp rpc -> /system $(Get-Date) ==="
& $adb connect $Dev 2>&1 | Out-Null

$sh = @(
  'mount -o rw,remount /system',
  '# copy the vendor originals over our renamed _system copies',
  'for l in libcdsprpc libadsprpc libsdsprpc; do',
  '  cp -f /vendor/lib/$l.so   /system/lib/$l.so   2>/dev/null',
  '  cp -f /vendor/lib64/$l.so /system/lib64/$l.so 2>/dev/null',
  'done',
  'chmod 644 /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so',
  'chown root:root /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so',
  'chcon u:object_r:system_lib_file:s0 /system/lib/lib?dsprpc.so /system/lib64/lib?dsprpc.so 2>/dev/null',
  'sync',
  'ls -l /system/lib/libcdsprpc.so',
  'echo "--- restart qvrd ---"',
  'stop pn2_qvrd; sleep 2; start pn2_qvrd; sleep 8',
  'grep VmRSS /proc/$(pidof qvrservice)/status',
  'echo "--- test ---"',
  'logcat -c',
  'timeout 12 /vendor/bin/qvrservicetest64 2>&1 | head -8',
  'sleep 1',
  'echo "--- qvr log ---"',
  'logcat -d | grep -iE "DspWrapper|QVRServiceTracker|VR mode|Plugin not valid|supportedTracking" | tail -10'
) -join "`n"
[System.IO.File]::WriteAllText('F:\PN2Lineage\tools\_vdsp.sh', $sh + "`n")
& $adb -s $Dev push F:\PN2Lineage\tools\_vdsp.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $Dev shell "su -c 'sh /data/local/tmp/_vdsp.sh'" 2>&1) | ForEach-Object { L ("   " + ($_ -replace '^\d\d-\d\d \S+\s+','')) }

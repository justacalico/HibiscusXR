# Which ABI does VRShell run as on stock vs on ours?
#
# Our tombstone says ABI 'arm64'. If stock runs the same app 32-bit, then our
# build is launching it in the wrong bitness -- the same defect already seen with
# CVService (PM recorded 64-bit, native libs 32-bit). A 64-bit process loading the
# 64-bit SDK takes a completely different code path from the one Pico ships and
# tests, which would explain a crash that survives every display/lens change.
$adb   = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
$log   = 'F:\PN2Lineage\notes\163_abi.log'
function L($m) { Add-Content $log $m; Write-Host $m }
Set-Content $log "=== VRShell ABI: ours vs stock ==="

& $adb connect $STOCK 2>&1 | Out-Null

foreach ($pair in @(@('OURS',$OURS), @('STOCK',$STOCK))) {
  $name = $pair[0]; $dev = $pair[1]
  L ""
  L "################ $name ################"
  # what the package manager recorded for the app
  $pkg = (& $adb -s $dev shell "dumpsys package com.pvr.vrshell | grep -iE 'primaryCpuAbi|secondaryCpuAbi|legacyNativeLibraryDir|codePath'" 2>&1)
  $pkg | ForEach-Object { L ("  pkg: " + $_.Trim()) }
  # what native libs the APK actually ships
  $libs = (& $adb -s $dev shell "su -c 'ls /system/app/VRShell/lib /system/priv-app/VRShell/lib /data/app/com.pvr.vrshell*/lib 2>/dev/null'" 2>&1)
  $libs | ForEach-Object { L ("  libdir: " + $_.Trim()) }
  # launch it and see which zygote it came from
  & $adb -s $dev shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
  Start-Sleep -Seconds 6
  $p = ((& $adb -s $dev shell "pidof com.pvr.vrshell" 2>&1) -join '').Trim()
  L ("  pid: " + $p)
  if ($p) {
    $exe = (& $adb -s $dev shell "su -c 'readlink /proc/$p/exe'" 2>&1)
    L ("  exe: " + ($exe -join '').Trim())
    $maps = (& $adb -s $dev shell "su -c 'grep -cE ""/lib64/"" /proc/$p/maps'" 2>&1)
    L ("  lib64 mappings: " + ($maps -join '').Trim())
    $pvr = (& $adb -s $dev shell "su -c 'grep -oE ""/system/lib(64)?/libPvr[^ ]*"" /proc/$p/maps | sort -u'" 2>&1)
    $pvr | ForEach-Object { L ("  pvr map: " + $_.Trim()) }
  }
}
L ""
L "=== done ==="

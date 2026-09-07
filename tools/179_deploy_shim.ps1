# Deploy the rebuilt shim (now with the null-format log guard) and retest VRShell.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$NDK = 'F:\Android\Sdk\ndk\23.1.7779620'
$RD  = "$NDK\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"
$log = 'F:\PN2Lineage\notes\179_deploy.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== shim redeploy $(Get-Date) ==="

L "--- is __android_log_print DEFINED (not UND) in the shim? ---"
(& $RD --dyn-syms F:\PN2Lineage\shim\libshim_pvr.so 2>&1 | Select-String '__android_log') |
    ForEach-Object { L ("   " + ($_ -replace '\s+',' ')) }

L "--- pushing ---"
& $adb -s $DEV push F:\PN2Lineage\shim\libshim_pvr.so /data/local/tmp/libshim_pvr.so 2>&1 | Select-Object -Last 1 | ForEach-Object { L "   $_" }
$sh = @'
mount -o rw,remount /system
cat /data/local/tmp/libshim_pvr.so > /system/lib64/libshim_pvr.so
chmod 644 /system/lib64/libshim_pvr.so
chown root:root /system/lib64/libshim_pvr.so
sync
ls -l /system/lib64/libshim_pvr.so
md5sum /system/lib64/libshim_pvr.so /data/local/tmp/libshim_pvr.so
'@
Set-Content -Encoding ASCII F:\PN2Lineage\tools\_shim.sh ($sh -replace "`r`n","`n")
& $adb -s $DEV push F:\PN2Lineage\tools\_shim.sh /data/local/tmp/ 2>&1 | Out-Null
(& $adb -s $DEV shell "su -c 'sh /data/local/tmp/_shim.sh'" 2>&1) | ForEach-Object { L "   $_" }

L "--- restarting pvrservice so it picks up the new shim ---"
& $adb -s $DEV shell "su -c 'pkill pvrservice'" 2>&1 | Out-Null
Start-Sleep -Seconds 6
L ("   pvrservice pid: " + ((& $adb -s $DEV shell "pidof pvrservice" 2>&1) -join '').Trim())

L "--- launching VRShell ---"
& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 15
$p = ((& $adb -s $DEV shell "pidof com.pvr.vrshell" 2>&1) -join '').Trim()
L ("   vrshell pid: $p  " + $(if ($p) { '*** SURVIVED ***' } else { 'died' }))
L ("   pvrservice : " + ((& $adb -s $DEV shell "pidof pvrservice" 2>&1) -join '').Trim())

L "--- how far did it get ---"
(& $adb -s $DEV shell "logcat -d | grep -iE 'EnterVrMode|hmdInfo|UnityPlugin|InitRenderThread|Unity |Fatal signal|binderdied|shim_pvr' | tail -30" 2>&1) |
    ForEach-Object { L ("   " + $_) }

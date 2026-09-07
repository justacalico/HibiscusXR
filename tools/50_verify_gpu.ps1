# Verify the vintf bind-mount replaces the vendor edit, then inventory graphics.
$adb='C:\adb\adb.exe'; $DEV='PA7B40NGE5300009W'
$log='F:\PN2Lineage\notes\50_verify.log'
function L($m){$s=("[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'),$m); Add-Content $log $s; Write-Host $s}
Set-Content $log "=== vintf overlay + gpu verify $(Get-Date) ==="

L "rebooting"; & $adb reboot | Out-Null; Start-Sleep -Seconds 20
for($i=0;$i -lt 150;$i++){ if((& $adb devices 2>&1)|Select-String "$DEV\s+device"){break}; Start-Sleep -Seconds 2 }
L "adb up"
$t0=Get-Date; $ok=$false
for($i=0;$i -lt 60;$i++){
  $bc=((& $adb shell getprop sys.boot_completed 2>&1) -join '').Trim()
  L ("t+{0,3}s up={1,4}s boot_completed='{2}'" -f [int]((Get-Date)-$t0).TotalSeconds, ((& $adb shell cut -d. -f1 /proc/uptime 2>&1)-join '').Trim(), $bc)
  if($bc -eq '1'){$ok=$true;break}; Start-Sleep -Seconds 5
}
L "boot_completed: $(if($ok){'YES'}else{'NO'})"
L ("watchdog kills: " + ((& $adb shell "logcat -d | grep -c 'WATCHDOG KILLING'" 2>&1)-join '').Trim())
L ("sound card    : " + ((& $adb shell "cat /proc/asound/cards" 2>&1)-join ' ').Trim())
L "--- is the bind mount live, and is vendor still pristine? ---"
(& $adb shell "grep -c 'hardware.boot' /vendor/manifest.xml; stat -c%s /vendor/manifest.xml; grep vendor/manifest /proc/mounts" 2>&1) | ForEach-Object { Add-Content $log ("   "+$_) }
L "=== done ==="

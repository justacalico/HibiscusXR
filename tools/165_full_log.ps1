# Full, ungrepped launch log for VRShell2 up to the fatal signal.
#
# Every previous look at this crash used a grep, which shows what MATCHED rather
# than what HAPPENED. The mechanism is now known (native call returns with x28
# trashed -> ART restores sp=0 -> faults at 0x10); what is still unknown is which
# native call. The last lines before the signal should name the subsystem, and any
# library-load failure or LD_PRELOAD surprise will show up here too.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$out = 'F:\PN2Lineage\notes\165_full.log'

& $adb -s $DEV shell "logcat -c" 2>&1 | Out-Null
& $adb -s $DEV shell "am start -n com.pvr.vrshell/.MainActivity" 2>&1 | Out-Null
Start-Sleep -Seconds 13

# everything from every buffer, unfiltered, then trimmed to the interesting window
$all = & $adb -s $DEV shell "logcat -d -b main,system,crash -v brief" 2>&1
Set-Content $out ($all -join "`n")
Write-Host ("captured " + $all.Count + " lines -> $out")

Write-Host ""
Write-Host "=== lines mentioning vrshell / pvr / unity / dlopen / linker ==="
$all | Where-Object { $_ -match 'vrshell|pvr|Pvr|unity|Unity|dlopen|linker|UnsatisfiedLink|nativeLibrary|LD_PRELOAD|il2cpp' } |
    Select-Object -Last 60 | ForEach-Object { Write-Host ("  " + $_) }

Write-Host ""
Write-Host "=== the last 25 lines from the vrshell process before it died ==="
$pidLine = $all | Where-Object { $_ -match 'Start proc .*com\.pvr\.vrshell' } | Select-Object -First 1
Write-Host ("  " + $pidLine)
$all | Where-Object { $_ -match 'F DEBUG|Fatal signal|died|Force finishing|ANR|Process .* exited' } |
    Select-Object -Last 25 | ForEach-Object { Write-Host ("  " + $_) }

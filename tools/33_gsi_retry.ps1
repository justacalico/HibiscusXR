# GSI retry, done properly.
#
# Recreates the ONE configuration that previously booted:
#   pristine GSI + current boot.img + EXISTING /data (no wipe)
# and captures logcat from the moment adb appears, so the bootloop cause is
# actually recorded this time instead of being guessed at.
$ErrorActionPreference = 'Continue'
$adb  = 'C:\adb\adb.exe'
$fb   = 'C:\adb\fastboot.exe'
$DEV  = 'PA7B40NGE5300009W'
$gsi  = 'F:\PN2Lineage\gsi\lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img'  # pristine sparse
$log  = 'F:\PN2Lineage\notes\gsi_retry.log'
$lc   = 'F:\PN2Lineage\notes\gsi_logcat.txt'
$km   = 'F:\PN2Lineage\notes\gsi_dmesg.txt'

function L($m) { Add-Content $log ("[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m) }
Set-Content $log "=== GSI retry $(Get-Date) ==="

# --- confirm state before we touch anything --------------------------------
$d = @((& $adb devices) | Select-String '\sdevice$' | ForEach-Object { ($_ -split '\s+')[0] })
if ($d -notcontains $DEV) { L "ABORT: $DEV not in adb"; exit 1 }
L "adb OK: $DEV"
L ("usb config now: " + ((& $adb shell getprop persist.sys.usb.config) -join '').Trim())
L ("release before : " + ((& $adb shell getprop ro.build.version.release) -join '').Trim())

L "rebooting to bootloader"
& $adb reboot bootloader | Out-Null
Start-Sleep -Seconds 8

for ($i=0; $i -lt 15; $i++) { $f=(& $fb devices) -join ' '; if ($f) { break }; Start-Sleep -Seconds 2 }
if (-not $f) { L "ABORT: no fastboot"; exit 1 }
L "fastboot: $f"

# unlock with a timeout - it wedges if the session has been idle
$p = Start-Process -FilePath $fb -ArgumentList 'oem','pico','unlock' -NoNewWindow -PassThru `
     -RedirectStandardOutput "$env:TEMP\u.out" -RedirectStandardError "$env:TEMP\u.err"
if (-not $p.WaitForExit(30000)) { $p.Kill(); L "ABORT: unlock hung"; exit 1 }
L ("unlock: " + ((Get-Content "$env:TEMP\u.out","$env:TEMP\u.err" -EA SilentlyContinue) -join ' '))

# --- flash system only. NO -w. /data stays. --------------------------------
L "flashing pristine GSI to system (no wipe)"
& $fb -S 128M flash system $gsi 2>&1 | Tee-Object -FilePath $log -Append | Out-Null
if ($LASTEXITCODE -ne 0) { L "ABORT: flash failed rc=$LASTEXITCODE, left in fastboot"; exit 1 }
L "flash OK"

& $fb reboot 2>&1 | Out-Null
L "rebooted - watching for adb"

# --- catch adb the instant it appears, then never let go -------------------
$t0 = Get-Date; $up = $false
for ($i=0; $i -lt 200; $i++) {
  $s = (& $adb devices 2>&1) | Select-String $DEV
  if ($s -and ($s -match "$DEV\s+device")) { $up = $true; break }
  Start-Sleep -Seconds 2
}
$el = [int]((Get-Date)-$t0).TotalSeconds
if (-not $up) { L "no adb after ${el}s - device did not come up"; exit 1 }
L "*** ADB UP after ${el}s - capturing ***"

# dump everything immediately, before any reboot can clear it
& $adb shell logcat -d -v threadtime  2>&1 | Out-File $lc -Encoding utf8
& $adb shell dmesg                    2>&1 | Out-File $km -Encoding utf8
L ("logcat lines: " + (Get-Content $lc -EA SilentlyContinue).Count)
L ("dmesg  lines: " + (Get-Content $km -EA SilentlyContinue).Count)

foreach ($k in 'sys.boot_completed','init.svc.bootanim','init.svc.zygote','init.svc.surfaceflinger','ro.build.version.release','ro.lineage.version') {
  L ("{0,-26} {1}" -f $k, ((& $adb shell getprop $k 2>&1) -join '').Trim())
}

# the actually useful bit: fatals and crashes
L "--- FATAL / crash lines ---"
Select-String -Path $lc -Pattern 'FATAL|E AndroidRuntime|died|crash|Fatal signal|system_server' -EA SilentlyContinue |
  Select-Object -First 40 | ForEach-Object { Add-Content $log ("   " + $_.Line) }

L "=== done ==="

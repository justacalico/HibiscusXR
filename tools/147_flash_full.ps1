# Flash system-pn2-full.img and verify it boots.
#
# Two hard-won constraints, both non-optional:
#   * `fastboot oem pico unlock` is required once per fastboot session, and it
#     wedges if the session has been sitting idle - hence the timeout + kill.
#   * `-S 128M` is required. The bootloader advertises max-download-size
#     536870912, but 512M chunks kill the USB link partway through
#     ("Write to device failed (no link)") and leave system half-written.
$adb = 'C:\adb\adb.exe'
$fb  = 'C:\adb\fastboot.exe'
$DEV = 'PA7B40NGE5300009W'
$IMG = 'F:\PN2Lineage\out\system-pn2-full.img'
$log = 'F:\PN2Lineage\notes\147_flash.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== flash full image $(Get-Date) ==="

L ("image: {0} bytes" -f (Get-Item $IMG).Length)

L "rebooting to bootloader"
& $adb -s $DEV reboot bootloader 2>&1 | Out-Null
Start-Sleep -Seconds 10
$f = $null
for ($i = 0; $i -lt 20; $i++) {
    $f = (& $fb devices 2>&1) -join ' '
    if ($f -match '\S') { break }
    Start-Sleep -Seconds 2
}
if (-not $f) { L "ABORT: never reached fastboot"; exit 1 }
L "fastboot: $f"

# unlock, with a timeout because it hangs on a stale session
$p = Start-Process -FilePath $fb -ArgumentList 'oem','pico','unlock' -NoNewWindow -PassThru `
     -RedirectStandardOutput "$env:TEMP\u.out" -RedirectStandardError "$env:TEMP\u.err"
if (-not $p.WaitForExit(45000)) {
    $p.Kill()
    L "ABORT: oem pico unlock hung. Power-cycle into fastboot and re-run."
    exit 1
}
L ("unlock: " + ((Get-Content "$env:TEMP\u.out","$env:TEMP\u.err" -EA SilentlyContinue) -join ' ').Trim())

L "flashing system (3.5GB in 128M chunks - this takes a few minutes)"
$t0 = Get-Date
& $fb -S 128M flash system $IMG 2>&1 | ForEach-Object { if ($_ -match 'Sending|Writing|OKAY|FAILED|error') { Add-Content $log ("    " + $_) } }
$rc = $LASTEXITCODE
L ("flash finished rc=$rc in " + [int]((Get-Date)-$t0).TotalSeconds + "s")
if ($rc -ne 0) { L "ABORT: flash failed, device left in fastboot"; exit 1 }

L "rebooting"
& $fb reboot 2>&1 | Out-Null

Start-Sleep -Seconds 40
& $adb -s $DEV wait-for-device 2>&1 | Out-Null
for ($i = 0; $i -lt 60; $i++) {
    $bc = ((& $adb -s $DEV shell getprop sys.boot_completed 2>&1) -join '').Trim()
    if ($bc -eq '1') { break }
    Start-Sleep -Seconds 5
}
L "boot_completed=$bc uptime=$(((& $adb -s $DEV shell cut -d. -f1 /proc/uptime 2>&1) -join '').Trim())s"

Start-Sleep -Seconds 15
L "--- does the port still work end to end ---"
foreach ($c in @(
    @('sound card',   'cat /proc/asound/cards'),
    @('audio_policy', 'service check media.audio_policy'),
    @('wm size',      'wm size'),
    @('pvrservice',   'pidof pvrservice'),
    @('system_server','pidof system_server'),
    @('wakelock',     'getprop init.svc.pn2_settings')
)) {
    L ("   {0,-14} {1}" -f $c[0], ((& $adb -s $DEV shell $c[1] 2>&1) -join ' ').Trim())
}
L ("   sensor aborts  " + ((& $adb -s $DEV shell "logcat -d -b crash | grep -c DEVICE_PRIVATE_BASE" 2>&1) -join '').Trim())
L "--- pico packages ---"
(& $adb -s $DEV shell "pm list packages 2>/dev/null | grep -icE 'pvr|pico'" 2>&1) | ForEach-Object { L "   $_ pico packages registered" }
L "=== done ==="

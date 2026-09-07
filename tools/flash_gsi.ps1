# Flash a Treble GSI to the non-Eye Pico Neo 2, system partition ONLY.
#
# Deliberately does NOT touch: vendor, firmware, boot, dtbo, persist, or any
# bootloader partition. The device keeps supplying its own proprietary blobs and
# its factory calibration.
#
#   .\flash_gsi.ps1 -WhatIf     # show what would happen
#   .\flash_gsi.ps1             # flash system, reboot, do NOT wipe data
#   .\flash_gsi.ps1 -WipeData   # also wipe userdata+cache (needed if FBE blocks boot)
param(
    [switch]$WipeData,
    [switch]$WhatIf
)

$adb      = 'C:\adb\adb.exe'
$fastboot = 'C:\adb\fastboot.exe'
$DEAD     = 'PA7B40NGE5300009W'
$gsi      = 'F:\PN2Lineage\gsi\lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img'

if (-not (Test-Path $gsi)) { Write-Host "ABORT: GSI not found at $gsi"; exit 1 }
Write-Host ("GSI: {0} ({1} MB)" -f (Split-Path $gsi -Leaf), [math]::Round((Get-Item $gsi).Length/1MB,2))

if ($WhatIf) {
    Write-Host ""
    Write-Host "would run:"
    Write-Host "  adb reboot bootloader"
    Write-Host "  fastboot oem pico unlock"
    Write-Host "  fastboot flash system `"$gsi`""
    if ($WipeData) { Write-Host "  fastboot -w" }
    Write-Host "  fastboot reboot"
    exit 0
}

# --- guard: right device, in adb ---------------------------------------------
$devs = @((& $adb devices) | Select-String '\sdevice$' | ForEach-Object { ($_ -split '\s+')[0] })
if ($devs.Count -eq 1 -and $devs[0] -eq $DEAD) {
    Write-Host "adb sees the non-Eye unit; rebooting to bootloader..."
    & $adb -s $DEAD reboot bootloader
    Start-Sleep -Seconds 8
} elseif ($devs.Count -gt 0) {
    Write-Host "ABORT: unexpected adb device(s): $($devs -join ', ')"
    exit 1
} else {
    Write-Host "no adb device; assuming already in fastboot"
}

# --- guard: something is in fastboot -----------------------------------------
for ($i = 0; $i -lt 10; $i++) {
    $fb = (& $fastboot devices) -join ' '
    if ($fb) { break }
    Start-Sleep -Seconds 2
}
if (-not $fb) { Write-Host "ABORT: nothing in fastboot after 20s"; exit 1 }
Write-Host "fastboot: $fb"
Write-Host ""

# --- Pico gates writes behind this, per fastboot session ----------------------
Write-Host "=== fastboot oem pico unlock ==="
& $fastboot oem pico unlock
Write-Host ""

# -S 128M is not optional here. The bootloader advertises max-download-size
# 536870912, so plain `fastboot flash` sends 512M chunks and the USB link dies
# partway through ("Write to device failed (no link)"). 128M chunks are stable.
Write-Host "=== flashing system (128M sparse chunks) ==="
& $fastboot -S 128M flash system $gsi
$rc = $LASTEXITCODE
if ($rc -ne 0) {
    Write-Host ""
    Write-Host "FLASH FAILED (rc=$rc). Device left in fastboot - NOT rebooting."
    Write-Host "Recover with: .\restore_stock.ps1"
    exit 1
}
Write-Host ""

if ($WipeData) {
    Write-Host "=== wiping userdata + cache ==="
    & $fastboot -w
    Write-Host ""
}

Write-Host "=== rebooting ==="
& $fastboot reboot
Write-Host ""
Write-Host "Watch for boot. If it loops, run:  .\restore_stock.ps1"

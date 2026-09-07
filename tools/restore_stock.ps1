# Restore the non-Eye Pico Neo 2 to stock from the local backup.
#
# Use this if a GSI or custom system fails to boot. Restores only what is needed
# to get back to a working stock system; it deliberately DOES NOT touch the
# bootloader chain (xbl/abl/tz/hyp/...) because writing an older bootloader than
# the anti-rollback fuse value is the one true hard-brick vector on sdm845.
#
#   .\restore_stock.ps1              # restore system + boot + dtbo + vbmeta
#   .\restore_stock.ps1 -All         # also restore vendor, oem, persist
#   .\restore_stock.ps1 -WhatIf      # print the commands, flash nothing
#
param(
    [switch]$All,
    [switch]$WhatIf
)

$ErrorActionPreference = 'Stop'
$fastboot = 'C:\adb\fastboot.exe'
$adb      = 'C:\adb\adb.exe'
$bk       = 'F:\PN2Lineage\backup_nonEye'
$ota      = 'F:\PN2Lineage\images\ota_4.1.3'
$DEAD     = 'PA7B40NGE5300009W'

function Say($m) { Write-Host $m }

# --- images to restore, in a safe order ---------------------------------------
$core = @(
    @{ part='boot';    file="$bk\boot.img"    },
    @{ part='dtbo';    file="$bk\dtbo.img"    },
    @{ part='vbmeta';  file="$bk\vbmeta.img"  },
    @{ part='recovery';file="$bk\recovery.img"}
)
# system came from the OTA (stock, unmodified) - the on-device dd of it failed,
# which is fine because the OTA copy is byte-identical stock content.
$system = @(
    @{ part='system';  file="$ota\system.img" }
)
$extra = @(
    @{ part='vendor';  file="$bk\vendor.img"  },
    @{ part='oem';     file="$ota\oem.bin"    },
    @{ part='persist'; file="$bk\persist.img" }
)

$plan = $core + $system
if ($All) { $plan += $extra }

Say "=== restore plan ==="
foreach ($p in $plan) {
    $ok = Test-Path $p.file
    "{0,-10} {1,-52} {2}" -f $p.part, $p.file, $(if ($ok) { 'present' } else { 'MISSING' })
}
Say ""

$missing = $plan | Where-Object { -not (Test-Path $_.file) }
if ($missing) {
    Say "ABORT: missing images above. Not flashing a partial restore."
    exit 1
}

if ($WhatIf) {
    Say "=== WhatIf: commands that would run ==="
    Say "$fastboot oem pico unlock"
    foreach ($p in $plan) { Say "$fastboot flash $($p.part) `"$($p.file)`"" }
    Say "$fastboot reboot"
    exit 0
}

# --- get to fastboot ----------------------------------------------------------
$adbDevs = @((& $adb devices) | Select-String '\sdevice$' | ForEach-Object { ($_ -split '\s+')[0] })
if ($adbDevs -contains $DEAD) {
    Say "rebooting $DEAD to bootloader..."
    & $adb -s $DEAD reboot bootloader
    Start-Sleep -Seconds 6
}

$fb = (& $fastboot devices) -join ' '
if (-not $fb) { Say "ABORT: no device in fastboot. Enter fastboot manually and re-run."; exit 1 }
Say "fastboot sees: $fb"

# Pico gates writes behind this, and it does NOT persist across fastboot boots.
Say "unlocking fastboot session..."
& $fastboot oem pico unlock
Say ""

$fail = 0
foreach ($p in $plan) {
    Say "--- flashing $($p.part) ---"
    & $fastboot flash $p.part $p.file
    if ($LASTEXITCODE -ne 0) { Say "FAILED: $($p.part)"; $fail++ }
}

Say ""
if ($fail -gt 0) {
    Say "$fail partition(s) failed. NOT rebooting - fix before leaving fastboot."
    exit 1
}

Say "all flashed OK, rebooting"
& $fastboot reboot

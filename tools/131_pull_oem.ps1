# Pull the /oem Pico apps off the device. They are NOT in the OTA system.img we
# extracted from - /oem is a separate partition - so the device is the only
# source. They are deodexed like everything else, so they need the same
# unquicken -> inject -> re-sign treatment.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$D   = 'PA7B40NGE5300009W'
$OUT = 'F:\PN2Lineage\oem_apps'

# launcher and home first; the media players are large and not on the path to VR
$WANT = @('PVRLauncher','PVRHome','store2d','provision2d','ToBToolService')

Remove-Item $OUT -Recurse -Force -EA SilentlyContinue
New-Item -ItemType Directory -Force -Path $OUT | Out-Null

foreach ($a in $WANT) {
    $dst = Join-Path $OUT $a
    New-Item -ItemType Directory -Force -Path $dst | Out-Null
    & $adb -s $D pull "/oem/priv-app/$a" $OUT 2>&1 | Out-Null
    $files = Get-ChildItem $dst -Recurse -File -EA SilentlyContinue
    $apk = $files | Where-Object { $_.Extension -eq '.apk' } | Select-Object -First 1
    $vdex = $files | Where-Object { $_.Extension -eq '.vdex' } | Select-Object -First 1
    $so  = ($files | Where-Object { $_.Extension -eq '.so' }).Count
    "{0,-16} apk={1,-9} vdex={2,-9} libs={3}" -f $a,
        $(if ($apk) { $apk.Length } else { 'MISSING' }),
        $(if ($vdex) { $vdex.Length } else { 'MISSING' }), $so
}
"`ntotal pulled: $([math]::Round((Get-ChildItem $OUT -Recurse -File | Measure-Object Length -Sum).Sum/1MB,1)) MB"

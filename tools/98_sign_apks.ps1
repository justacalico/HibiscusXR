# Align + platform-sign the dex-injected APKs, then verify with aapt2.
#
# The aapt2 check is not optional: it is what caught the corrupted archives the
# .NET ZipArchive round produced. An apk that apksigner happily signs can still
# be unreadable to the platform.
$ErrorActionPreference = 'Continue'
$AS   = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$ZA   = 'F:\Android\Sdk\build-tools\34.0.0\zipalign.exe'
$AAPT = 'F:\Android\Sdk\build-tools\34.0.0\aapt2.exe'
$KEY  = 'F:\PN2Lineage\build\keys'
$SRC  = 'F:\PN2Lineage\pvr_apps_injected'
$OUT  = 'F:\PN2Lineage\pvr_apps_final'
$PLAT = 'c8a2e9bccf597c2fb6dc66bee293fc13f2fc47ec77bc6b2b0d52c11f51192ab8'

Remove-Item $OUT -Recurse -Force -EA SilentlyContinue
New-Item -ItemType Directory -Force -Path $OUT | Out-Null

$good = 0; $bad = 0
foreach ($d in (Get-ChildItem $SRC -Directory)) {
    $apk = Get-ChildItem $d.FullName -Filter '*.apk' | Select-Object -First 1
    if (-not $apk) { continue }
    $dst = Join-Path $OUT $d.Name
    New-Item -ItemType Directory -Force -Path $dst | Out-Null

    $aligned = Join-Path $dst 'aligned.apk'
    & $ZA -p -f 4 $apk.FullName $aligned 2>&1 | Out-Null
    $final = Join-Path $dst $apk.Name
    & $AS sign --key "$KEY\platform.pk8" --cert "$KEY\platform.x509.pem" `
               --out $final $aligned 2>&1 | Out-Null
    Remove-Item $aligned -Force -EA SilentlyContinue

    if (-not (Test-Path $final)) { "  {0,-22} SIGN FAILED" -f $d.Name; $bad++; continue }

    # does the platform tooling actually accept it?
    $badge = & $AAPT dump badging $final 2>&1 | Out-String
    $pkg   = if ($badge -match "package: name='([^']+)'") { $Matches[1] } else { '' }
    $certs = & $AS verify --print-certs $final 2>&1 | Out-String
    $okC   = $certs -match $PLAT
    $ok    = $pkg -and $okC

    if ($ok) { $good++ } else { $bad++ }
    "  {0,-22} {1,7} KB  {2,-34} {3}" -f `
        $d.Name, [math]::Round((Get-Item $final).Length/1KB), $pkg, `
        $(if ($ok) { 'aapt2 OK / platform cert OK' } else { 'REJECTED' })
}
"`ngood=$good bad=$bad   total $([math]::Round((Get-ChildItem $OUT -Recurse -File | Measure-Object Length -Sum).Sum/1MB,1)) MB"

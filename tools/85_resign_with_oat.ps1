# Rebuild the install tree, this time WITH the oat/ directories.
#
# The first attempt shipped only the .apk and the apps had no bytecode at all:
#   java.io.IOException: No original dex files found for dex location ...
# These apks are deodexed on stock - the dex lives in oat/<arch>/*.vdex, with the
# compiled code in the matching .odex. Dropping oat/ left empty shells, and since
# com.pvr.pxrnotification is a persistent process, the resulting crash loop locked
# the device up.
#
# Open question this tests: does re-signing the apk invalidate the odex? The odex
# records dex checksums, and these apks contain no dex, so the vdex is the source
# of truth and the apk signature should not participate. If ART still rejects it
# we have to properly deodex (pull dex out of the vdex, put it back in the apk).
$ErrorActionPreference = 'Continue'
$AS     = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$ZA     = 'F:\Android\Sdk\build-tools\34.0.0\zipalign.exe'
$KEYDIR = 'F:\PN2Lineage\build\keys'
$SRC    = 'F:\PN2Lineage\pvr_apps'
$OUT    = 'F:\PN2Lineage\pvr_apps_signed'

# needs re-signing (sharedUserId=android.uid.system)
$CORE = @('PicoSettingsProvider','configserverservice','CVService','pvrdisplay',
          'pvr_adapter','PxrNotification','PVRVerify','ShortcutMenu','VRShell2')
# signature-compatible already
$ASIS = @('PicoToSvrService','InitServer','VRUserCenter2')

Remove-Item $OUT -Recurse -Force -EA SilentlyContinue
New-Item -ItemType Directory -Force -Path $OUT | Out-Null

function SrcDir($name) {
    Get-ChildItem $SRC -Recurse -Directory | Where-Object { $_.Name -eq $name } | Select-Object -First 1
}

foreach ($n in ($CORE + $ASIS)) {
    $sd = SrcDir $n
    if (-not $sd) { "  {0,-24} NOT FOUND" -f $n; continue }
    $apk = Get-ChildItem $sd.FullName -Filter '*.apk' | Select-Object -First 1
    if (-not $apk) { "  {0,-24} NO APK" -f $n; continue }

    $dst = Join-Path $OUT $n
    New-Item -ItemType Directory -Force -Path $dst | Out-Null

    if ($CORE -contains $n) {
        $aligned = Join-Path $dst 'aligned.apk'
        & $ZA -p -f 4 $apk.FullName $aligned | Out-Null
        & $AS sign --key (Join-Path $KEYDIR 'platform.pk8') `
                   --cert (Join-Path $KEYDIR 'platform.x509.pem') `
                   --out (Join-Path $dst $apk.Name) $aligned 2>&1 | Out-Null
        Remove-Item $aligned -Force -EA SilentlyContinue
        $mode = 're-signed'
    } else {
        Copy-Item $apk.FullName (Join-Path $dst $apk.Name)
        $mode = 'as-is'
    }

    # THE FIX: bring the oat/ tree along
    $oat = Join-Path $sd.FullName 'oat'
    $oatN = 0
    if (Test-Path $oat) {
        Copy-Item $oat (Join-Path $dst 'oat') -Recurse -Force
        $oatN = (Get-ChildItem (Join-Path $dst 'oat') -Recurse -File).Count
    }
    "  {0,-24} {1,-10} oat files: {2}" -f $n, $mode, $oatN
}

"`n=== resulting tree ==="
Get-ChildItem $OUT -Recurse -File |
    Group-Object { $_.Directory.Parent.Name } |
    ForEach-Object { "  {0,-24} {1} files" -f $_.Name, $_.Count } | Select-Object -First 20
"total: $((Get-ChildItem $OUT -Recurse -File).Count) files, $([math]::Round((Get-ChildItem $OUT -Recurse -File | Measure-Object Length -Sum).Sum/1MB,1)) MB"

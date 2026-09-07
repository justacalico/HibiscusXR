# Put the unquickened dex back into each APK and re-sign.
#
# These apks shipped deodexed: no classes.dex at all, code lived in oat/*.vdex.
# vdexExtractor has now reverted the quickened bytecode to plain dex, so the apks
# can be made self-contained the way a normal (odex-free) build would be. Android
# 10 then just JITs/AOTs them itself.
#
# Every apk gets re-signed with the AOSP platform key - including the three that
# did not need it before, because adding an entry to the zip invalidates Pico's
# original signature regardless.
$ErrorActionPreference = 'Continue'
$AS     = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$ZA     = 'F:\Android\Sdk\build-tools\34.0.0\zipalign.exe'
$KEYDIR = 'F:\PN2Lineage\build\keys'
$APPS   = 'F:\PN2Lineage\pvr_apps'
$DEX    = 'F:\PN2Lineage\pvr_dex'
$OUT    = 'F:\PN2Lineage\pvr_apps_dexed'
$PLATCERT = 'c8a2e9bccf597c2fb6dc66bee293fc13f2fc47ec77bc6b2b0d52c11f51192ab8'

# core runtime set; skipping the big optional ones for now
$WANT = @('PicoSettingsProvider','configserverservice','CVService','pvrdisplay',
          'pvr_adapter','PxrNotification','PVRVerify','ShortcutMenu','VRShell2',
          'PicoToSvrService','InitServer','VRUserCenter2')

Remove-Item $OUT -Recurse -Force -EA SilentlyContinue
New-Item -ItemType Directory -Force -Path $OUT | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem

foreach ($n in $WANT) {
    $srcApk = Get-ChildItem $APPS -Recurse -Filter '*.apk' |
              Where-Object { $_.Directory.Name -eq $n } | Select-Object -First 1
    $dexF   = Join-Path $DEX "$n\${n}_classes.dex"
    if (-not $srcApk -or -not (Test-Path $dexF)) { "  {0,-22} MISSING apk or dex" -f $n; continue }

    $dstDir = Join-Path $OUT $n
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    $work   = Join-Path $dstDir 'work.apk'
    Copy-Item $srcApk.FullName $work -Force

    # inject classes.dex at the zip root
    $zip = [System.IO.Compression.ZipFile]::Open($work, 'Update')
    try {
        $old = $zip.GetEntry('classes.dex')
        if ($old) { $old.Delete() }
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $dexF, 'classes.dex') | Out-Null
    } finally { $zip.Dispose() }

    $aligned = Join-Path $dstDir 'aligned.apk'
    & $ZA -p -f 4 $work $aligned 2>&1 | Out-Null
    if (-not (Test-Path $aligned)) { "  {0,-22} ZIPALIGN FAILED" -f $n; continue }

    $final = Join-Path $dstDir $srcApk.Name
    & $AS sign --key (Join-Path $KEYDIR 'platform.pk8') `
               --cert (Join-Path $KEYDIR 'platform.x509.pem') `
               --out $final $aligned 2>&1 | Out-Null
    Remove-Item $work, $aligned -Force -EA SilentlyContinue
    if (-not (Test-Path $final)) { "  {0,-22} SIGN FAILED" -f $n; continue }

    # verify: correct cert AND classes.dex really present
    $certs = (& $AS verify --print-certs $final 2>&1 | Out-String)
    $okCert = $certs -match $PLATCERT
    $z2 = [System.IO.Compression.ZipFile]::OpenRead($final)
    $dexEntry = $z2.Entries | Where-Object { $_.FullName -eq 'classes.dex' }
    $dexLen = if ($dexEntry) { $dexEntry.Length } else { 0 }
    $z2.Dispose()

    "  {0,-22} {1,8} KB  dex={2,-9} cert={3}" -f `
        $n, [math]::Round((Get-Item $final).Length/1KB), $dexLen, $(if ($okCert) {'platform OK'} else {'MISMATCH'})
}

"`ntotal: $([math]::Round((Get-ChildItem $OUT -Recurse -File | Measure-Object Length -Sum).Sum/1MB,1)) MB"

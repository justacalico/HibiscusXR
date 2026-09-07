# Re-sign Pico's system apps with the AOSP platform key so they can legitimately
# hold android.uid.system on this GSI.
#
# All of them get the SAME key, so their inter-app signature checks still pass.
# We are not redistributing these: the end user extracts them from their own
# device and runs this step locally. See overlay/PROPRIETARY-PVR.md.
#
# Core runtime only for now. Deliberately skipped:
#   seethrough.setting  193 MB, passthrough UI, not needed to prove the runtime
#   picovr.wing          23 MB, Pico's store/launcher shell
#   assistanthmd          6 MB, setup assistant
#   picostreamassistant   2 MB, streaming
# Add them once the core works.
$ErrorActionPreference = 'Stop'
$AS      = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$ZA      = 'F:\Android\Sdk\build-tools\34.0.0\zipalign.exe'
$KEYDIR  = 'F:\PN2Lineage\build\keys'
$SRC     = 'F:\PN2Lineage\pvr_apps'
$OUT     = 'F:\PN2Lineage\pvr_apps_signed'

# dir name -> apk name, in dependency-ish order
$CORE = @(
  'PicoSettingsProvider',   # settings provider other pico apps read
  'configserverservice',    # com.pvr.configuration
  'CVService',              # controller tracking
  'pvrdisplay',             # display service
  'pvr_adapter',
  'PxrNotification',
  'PVRVerify',
  'ShortcutMenu',
  'VRShell2'                # the VR launcher itself
)
# already signature-compatible (no sharedUserId) - copy without re-signing
$ASIS = @('PicoToSvrService', 'InitServer', 'VRUserCenter2')

Remove-Item $OUT -Recurse -Force -EA SilentlyContinue
New-Item -ItemType Directory -Force -Path $OUT | Out-Null

function FindApk($name) {
    Get-ChildItem $SRC -Recurse -Filter '*.apk' |
        Where-Object { $_.Directory.Name -eq $name } | Select-Object -First 1
}

$total = 0
foreach ($n in $CORE) {
    $apk = FindApk $n
    if (-not $apk) { "  {0,-24} NOT FOUND" -f $n; continue }
    $dstDir = Join-Path $OUT $n
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    $aligned = Join-Path $dstDir "aligned.apk"
    $signed  = Join-Path $dstDir $apk.Name

    # zipalign first; apksigner wants aligned input for v2
    & $ZA -p -f 4 $apk.FullName $aligned
    if ($LASTEXITCODE -ne 0) { "  {0,-24} ZIPALIGN FAILED" -f $n; continue }

    & $AS sign --key (Join-Path $KEYDIR 'platform.pk8') `
               --cert (Join-Path $KEYDIR 'platform.x509.pem') `
               --out $signed $aligned 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { "  {0,-24} SIGN FAILED" -f $n; continue }
    Remove-Item $aligned -Force

    $sha = (& $AS verify --print-certs $signed 2>&1 | Out-String)
    $ok  = $sha -match 'c8a2e9bccf597c2fb6dc66bee293fc13f2fc47ec77bc6b2b0d52c11f51192ab8'
    $mb  = [math]::Round((Get-Item $signed).Length / 1MB, 1)
    $total += $mb
    "  {0,-24} {1,7} MB  {2}" -f $n, $mb, $(if ($ok) { 'signed platform OK' } else { 'CERT MISMATCH' })
}

"`n=== copied unchanged (no sharedUserId) ==="
foreach ($n in $ASIS) {
    $apk = FindApk $n
    if (-not $apk) { "  {0,-24} NOT FOUND" -f $n; continue }
    $dstDir = Join-Path $OUT $n
    New-Item -ItemType Directory -Force -Path $dstDir | Out-Null
    Copy-Item $apk.FullName (Join-Path $dstDir $apk.Name)
    $mb = [math]::Round((Get-Item $apk.FullName).Length / 1MB, 1)
    $total += $mb
    "  {0,-24} {1,7} MB" -f $n, $mb
}

"`ntotal to install: $([math]::Round($total,1)) MB"

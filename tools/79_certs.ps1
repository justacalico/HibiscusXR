# Confirm the platform-key mismatch is real before designing around it, and find
# out whether we can re-sign.
#
# If the GSI is signed with AOSP test-keys, those keys are PUBLIC (they live in
# AOSP at build/target/product/security/). Re-signing Pico's apps with the same
# platform key would let them hold android.uid.system legitimately. That hinges
# on us actually having platform.pk8 + platform.x509.pem.
$AS   = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$adb  = 'C:\adb\adb.exe'
$tmp  = 'F:\PN2Lineage\pvr_apps\_certs'
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

# a GSI system app that definitely holds android.uid.system
& $adb pull /system/priv-app/SettingsProvider/SettingsProvider.apk "$tmp\gsi_SettingsProvider.apk" 2>&1 | Out-Null

function CertOf($apk, $label) {
    if (-not (Test-Path $apk)) { "$label : MISSING"; return }
    $out = & $AS verify --print-certs $apk 2>&1 | Out-String
    $sha = if ($out -match 'certificate SHA-256 digest: ([0-9a-f]+)') { $Matches[1] } else { 'unknown' }
    $dn  = if ($out -match 'Signer #1 certificate DN: (.+)')          { $Matches[1].Trim() } else { 'unknown' }
    "{0,-26} {1}" -f $label, $sha
    "{0,-26} {1}" -f ''     , $dn
}

"=== signing certificates ==="
CertOf "$tmp\gsi_SettingsProvider.apk"                                  'GSI SettingsProvider'
CertOf 'F:\PN2Lineage\pvr_apps\priv-app\VRShell2\VRShell2.apk'          'Pico VRShell2'
CertOf 'F:\PN2Lineage\pvr_apps\priv-app\pvrdisplay\pvrdisplay.apk'      'Pico pvrdisplay'
CertOf 'F:\PN2Lineage\pvr_apps\priv-app\CVService\CVService.apk'        'Pico CVService'

"`n=== do we have AOSP platform signing keys anywhere? ==="
$hits = @()
foreach ($root in 'F:\Android', 'F:\PN2Lineage') {
    $hits += Get-ChildItem $root -Recurse -Include 'platform.pk8','platform.x509.pem','testkey.pk8' -EA SilentlyContinue
}
if ($hits) { $hits | ForEach-Object { "  " + $_.FullName } } else { "  none found locally" }

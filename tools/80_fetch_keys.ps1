# Fetch the AOSP platform signing key so Pico's system apps can be re-signed to
# match this GSI's platform identity.
#
# These are the PUBLIC AOSP test keys, from the canonical android.googlesource.com
# repo, pinned to an Android 10 release tag. They are not a secret and not a
# security boundary - anything signed with them is publicly forgeable. Fine for a
# development image; a shipping ROM must be signed with a key we generate and keep
# private, and Pico's apps re-signed with that instead.
#
# Kept in F:\PN2Lineage\build\keys, outside anything that would be committed.
#
# The verification at the end is the point of the whole script: the fetched
# certificate MUST hash to the same SHA-256 that apksigner reported for the GSI's
# own SettingsProvider. If it does not, this is the wrong key and re-signing
# would achieve nothing.
$ErrorActionPreference = 'Stop'
$TAG      = 'android-10.0.0_r47'
$BASE     = "https://android.googlesource.com/platform/build/+/refs/tags/$TAG/target/product/security"
$KEYDIR   = 'F:\PN2Lineage\build\keys'
# from tools/79_certs.ps1 - the GSI's own platform signer
$EXPECTED = 'c8a2e9bccf597c2fb6dc66bee293fc13f2fc47ec77bc6b2b0d52c11f51192ab8'

New-Item -ItemType Directory -Force -Path $KEYDIR | Out-Null

function Fetch($name) {
    # build by concatenation and print it: interpolating "?format=TEXT" directly
    # into the string was producing a URL the server 404'd on, while the same URL
    # typed literally worked
    $url = $BASE + '/' + $name + '?format=TEXT'
    $dst = Join-Path $KEYDIR $name
    Write-Host "fetching $url"
    $b64 = (Invoke-WebRequest -Uri $url -UseBasicParsing).Content
    [System.IO.File]::WriteAllBytes($dst, [Convert]::FromBase64String($b64))
    "  {0,-22} {1} bytes" -f $name, (Get-Item $dst).Length
}

Fetch 'platform.pk8'
Fetch 'platform.x509.pem'

"`n=== verify this is really the GSI's platform key ==="
$cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2(
            (Join-Path $KEYDIR 'platform.x509.pem'))
$sha  = [System.BitConverter]::ToString(
            [System.Security.Cryptography.SHA256]::Create().ComputeHash($cert.GetRawCertData())
        ).Replace('-','').ToLower()
"  subject : $($cert.Subject)"
"  sha256  : $sha"
"  expected: $EXPECTED"
if ($sha -eq $EXPECTED) {
    Write-Host "  MATCH - re-signing with this key will satisfy android.uid.system" -ForegroundColor Green
} else {
    Write-Host "  MISMATCH - wrong key, do NOT use it" -ForegroundColor Red
    exit 1
}

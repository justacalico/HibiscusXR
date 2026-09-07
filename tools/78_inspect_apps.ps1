# Before installing any Pico app, find out which ones can actually work here.
#
# The decisive attribute is sharedUserId. An app declaring
# android:sharedUserId="android.uid.system" must be signed with the SAME platform
# key as the framework. Pico's apps are signed with Pico's key; this GSI is
# signed with test-keys. Those apps cannot be granted system uid and will be
# rejected - no amount of copying fixes it.
$AAPT = 'F:\Android\Sdk\build-tools\34.0.0\aapt2.exe'
$ROOT = 'F:\PN2Lineage\pvr_apps'

$rows = @()
Get-ChildItem $ROOT -Recurse -Filter *.apk | ForEach-Object {
    $apk = $_.FullName
    $dump = & $AAPT dump badging $apk 2>&1 | Out-String
    $xml  = & $AAPT dump xmltree --file AndroidManifest.xml $apk 2>&1 | Out-String

    $pkg = if ($dump -match "package: name='([^']+)'") { $Matches[1] } else { '?' }
    $sdk = if ($dump -match "sdkVersion:'(\d+)'") { $Matches[1] } else { '' }
    $tgt = if ($dump -match "targetSdkVersion:'(\d+)'") { $Matches[1] } else { '' }
    $shared = if ($xml -match 'sharedUserId[^"]*"([^"]+)"') { $Matches[1] } else { '' }

    $rows += [pscustomobject]@{
        Apk        = $_.FullName.Substring($ROOT.Length + 1)
        Package    = $pkg
        MinSdk     = $sdk
        TargetSdk  = $tgt
        SharedUid  = $shared
        MB         = [math]::Round($_.Length / 1MB, 1)
    }
}

"`n=== Pico VR apps ==="
$rows | Sort-Object SharedUid, Package | Format-Table -AutoSize Package, MinSdk, TargetSdk, SharedUid, MB

$blocked = $rows | Where-Object { $_.SharedUid -eq 'android.uid.system' }
$ok      = $rows | Where-Object { $_.SharedUid -ne 'android.uid.system' }
"BLOCKED by platform-key mismatch (sharedUserId=android.uid.system): $($blocked.Count)"
$blocked | ForEach-Object { "   $($_.Package)" }
"INSTALLABLE as-is: $($ok.Count)"
"total size: $([math]::Round(($rows | Measure-Object MB -Sum).Sum,1)) MB"

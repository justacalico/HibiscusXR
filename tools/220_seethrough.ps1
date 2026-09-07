# Install the see-through (passthrough) calibration app.
#
# com.pvr.seethrough.setting is the 6DoF/passthrough calibration the shell wants.
# Two things make it simpler than the earlier apps: there is no oat/ directory, so
# the APK still carries classes.dex and needs no deodexing; but it declares
# sharedUser=android.uid.system, so it MUST carry the same signature as the
# platform. Ours is AOSP test-keys, so it has to be re-signed regardless.
#
# It is 203 MB, almost all Unity assets.
$ErrorActionPreference = 'Continue'
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$OURS  = '192.168.0.172:5555'
$AS    = 'F:\Android\Sdk\build-tools\34.0.0\apksigner.bat'
$ZA    = 'F:\Android\Sdk\build-tools\34.0.0\zipalign.exe'
$AAPT  = 'F:\Android\Sdk\build-tools\34.0.0\aapt2.exe'
$KEY   = 'F:\PN2Lineage\build\keys'
$PLAT  = 'c8a2e9bccf597c2fb6dc66bee293fc13f2fc47ec77bc6b2b0d52c11f51192ab8'
$work  = 'F:\PN2Lineage\seethrough'
$log   = 'F:\PN2Lineage\notes\220_seethrough.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== seethrough setting install $(Get-Date) ==="
New-Item -ItemType Directory -Force "$work\lib\arm64" | Out-Null

& $adb connect $STOCK 2>&1 | Out-Null
& $adb connect $OURS  2>&1 | Out-Null

if (-not (Test-Path "$work\seethroughsetting.apk")) {
    L "pulling the apk from stock (203 MB, over wifi - takes a moment)"
    & $adb -s $STOCK pull /system/priv-app/seethroughsetting/seethroughsetting.apk "$work\seethroughsetting.apk" 2>&1 |
        Select-Object -Last 1 | ForEach-Object { L "   $_" }
}
$libs = @('libPvr_UnitySDK.so','libil2cpp.so','libmain.so','libnative-lib.so','libnative.so','libtracking_module.so','libunity.so')
foreach ($l in $libs) {
    if (-not (Test-Path "$work\lib\arm64\$l")) {
        & $adb -s $STOCK pull "/system/priv-app/seethroughsetting/lib/arm64/$l" "$work\lib\arm64\$l" 2>&1 | Out-Null
    }
}
L ("libs staged: " + (Get-ChildItem "$work\lib\arm64" -Filter *.so).Count + "/" + $libs.Count)

# does it really carry dex (i.e. no deodex needed)?
$hasDex = (& $AAPT dump badging "$work\seethroughsetting.apk" 2>&1 | Out-String) -match "package: name='([^']+)'"
$pkgName = if ($hasDex) { $Matches[1] } else { '' }
L "package: $pkgName"

L "aligning + platform-signing"
$aligned = "$work\aligned.apk"
Remove-Item $aligned -Force -EA SilentlyContinue
& $ZA -p -f 4 "$work\seethroughsetting.apk" $aligned 2>&1 | Out-Null
$final = "$work\seethroughsetting-signed.apk"
Remove-Item $final -Force -EA SilentlyContinue
& $AS sign --key "$KEY\platform.pk8" --cert "$KEY\platform.x509.pem" --out $final $aligned 2>&1 | Out-Null
Remove-Item $aligned -Force -EA SilentlyContinue

if (-not (Test-Path $final)) { L "SIGN FAILED"; exit 1 }
$certs = & $AS verify --print-certs $final 2>&1 | Out-String
$badge = & $AAPT dump badging $final 2>&1 | Out-String
$okC = $certs -match $PLAT
$okP = $badge -match "package: name='com.pvr.seethrough.setting'"
L ("signed: {0} MB   platform cert: {1}   aapt2: {2}" -f `
    [math]::Round((Get-Item $final).Length/1MB,1), $(if($okC){'OK'}else{'BAD'}), $(if($okP){'OK'}else{'BAD'}))
if (-not ($okC -and $okP)) { L "refusing to install a bad apk"; exit 1 }

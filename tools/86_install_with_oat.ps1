# Install the Pico apps WITH their oat/ trees this time.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$SRC = 'F:\PN2Lineage\pvr_apps'          # to learn app vs priv-app
$SGN = 'F:\PN2Lineage\pvr_apps_signed'   # what we install

$loc = @{}
Get-ChildItem $SRC -Recurse -Filter '*.apk' | ForEach-Object {
    $loc[$_.Directory.Name] = $_.Directory.Parent.Name
}

& $adb wait-for-device
& $adb shell rm -rf /data/local/tmp/pvrapps2 2>&1 | Out-Null
& $adb shell mkdir -p /data/local/tmp/pvrapps2 2>&1 | Out-Null

Write-Host "pushing (112 MB, takes a moment)..."
& $adb push $SGN /data/local/tmp/pvrapps2 2>&1 | Select-Object -Last 1

$lines = @('#!/system/bin/sh', 'exec 2>&1', 'mount -o rw,remount /system')
foreach ($d in (Get-ChildItem $SGN -Directory)) {
    $n     = $d.Name
    $where = if ($loc.ContainsKey($n)) { $loc[$n] } else { 'priv-app' }
    $dest  = "/system/$where/$n"
    $lines += "rm -rf $dest"
    $lines += "cp -r /data/local/tmp/pvrapps2/pvr_apps_signed/$n $dest"
}
# ownership and modes: cp -r from /data/local/tmp carries shell:shell across,
# which is what broke the first PVR lib install
$lines += 'for w in priv-app app; do'
$lines += '  for d in configserverservice CVService InitServer PicoSettingsProvider pvrdisplay PVRVerify pvr_adapter ShortcutMenu VRShell2 VRUserCenter2 PicoToSvrService PxrNotification; do'
$lines += '    [ -d /system/$w/$d ] || continue'
$lines += '    chown -R root:root /system/$w/$d'
$lines += '    find /system/$w/$d -type d -exec chmod 755 {} \;'
$lines += '    find /system/$w/$d -type f -exec chmod 644 {} \;'
$lines += '  done'
$lines += 'done'
$lines += 'sync'
$lines += 'mount -o ro,remount /system'
$lines += 'echo "--- installed, with oat ---"'
$lines += 'for w in priv-app app; do'
$lines += '  for d in /system/$w/*/; do'
$lines += '    case "$d" in *VRShell2*|*pvrdisplay*|*CVService*|*PxrNotification*|*pvr_adapter*|*configserver*|*PicoSettings*|*PVRVerify*|*ShortcutMenu*|*InitServer*|*VRUserCenter2*|*PicoToSvr*)'
$lines += '      echo "$d  apk=$(ls $d*.apk 2>/dev/null | wc -l)  oat=$(find $d/oat -type f 2>/dev/null | wc -l)" ;;'
$lines += '    esac'
$lines += '  done'
$lines += 'done'

$sh = 'F:\PN2Lineage\tools\_install_oat_dev.sh'
[System.IO.File]::WriteAllText($sh, ($lines -join "`n") + "`n")
& $adb push $sh /data/local/tmp/install_oat.sh 2>&1 | Out-Null
Write-Host "`n=== installing ==="
& $adb shell "chmod 755 /data/local/tmp/install_oat.sh; /system/xbin/su -c /data/local/tmp/install_oat.sh" 2>&1

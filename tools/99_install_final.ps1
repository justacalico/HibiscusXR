# Install the deodexed + platform-signed Pico apps.
#
# PxrNotification is held back on purpose: it declares android:persistent, so if
# it fails ActivityManager restarts it several times a second at system uid and
# takes the device down, which is exactly what happened before. Prove the other
# 11 first, then add it.
#
# No oat/ this time. The apks now carry classes.dex, so ART compiles them itself;
# shipping Pico's 8.1 odex alongside would only give ART a stale artifact to
# reject.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$SRC = 'F:\PN2Lineage\pvr_apps'
$FIN = 'F:\PN2Lineage\pvr_apps_final'
$HOLD = @('PxrNotification')

$loc = @{}
Get-ChildItem $SRC -Recurse -Filter '*.apk' | ForEach-Object {
    $loc[$_.Directory.Name] = $_.Directory.Parent.Name
}

$plan = Get-ChildItem $FIN -Directory | Where-Object { $HOLD -notcontains $_.Name }
"installing $($plan.Count) apps (holding back: $($HOLD -join ', '))"

& $adb shell rm -rf /data/local/tmp/pvrfinal 2>&1 | Out-Null
& $adb shell mkdir -p /data/local/tmp/pvrfinal 2>&1 | Out-Null
foreach ($d in $plan) {
    $apk = Get-ChildItem $d.FullName -Filter '*.apk' | Select-Object -First 1
    & $adb push $apk.FullName "/data/local/tmp/pvrfinal/$($d.Name).apk" 2>&1 | Out-Null
}

$lines = @('#!/system/bin/sh', 'exec 2>&1', 'mount -o rw,remount /system')
foreach ($d in $plan) {
    $where = if ($loc.ContainsKey($d.Name)) { $loc[$d.Name] } else { 'priv-app' }
    $dest  = "/system/$where/$($d.Name)"
    $lines += "rm -rf $dest"
    $lines += "mkdir -p $dest"
    $lines += "cp /data/local/tmp/pvrfinal/$($d.Name).apk $dest/$($d.Name).apk"
    $lines += "chmod 755 $dest"
    $lines += "chmod 644 $dest/$($d.Name).apk"
    $lines += "chown -R root:root $dest"
}
$lines += 'sync'
$lines += 'mount -o ro,remount /system'
$lines += 'echo "--- installed ---"'
$lines += 'ls -d /system/priv-app/*/ /system/app/*/ 2>/dev/null | grep -iE "pvr|pico|CVService|ShortcutMenu|InitServer|configserver"'

$sh = 'F:\PN2Lineage\tools\_install_final_dev.sh'
[System.IO.File]::WriteAllText($sh, ($lines -join "`n") + "`n")
& $adb push $sh /data/local/tmp/install_final.sh 2>&1 | Out-Null
& $adb shell "chmod 755 /data/local/tmp/install_final.sh; /system/xbin/su -c /data/local/tmp/install_final.sh" 2>&1

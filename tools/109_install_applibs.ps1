# Install the app-private native libs next to each apk, and undo the system
# libPvr_UnitySDK.so swap.
#
# That swap was a workaround for VRShell's missing app-private lib dir. VRShell
# ships its own libPvr_UnitySDK.so (2051960 bytes, matching no system variant),
# so with lib/arm64 in place the system copy should be back to stock.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$SRC = 'F:\PN2Lineage\pvr_applibs'

# where each app lives
$loc = @{}
Get-ChildItem 'F:\PN2Lineage\pvr_apps' -Recurse -Filter '*.apk' | ForEach-Object {
    $loc[$_.Directory.Name] = $_.Directory.Parent.Name
}

& $adb shell rm -rf /data/local/tmp/applibs 2>&1 | Out-Null
& $adb push $SRC /data/local/tmp/applibs 2>&1 | Select-Object -Last 1

$lines = @('#!/system/bin/sh', 'exec 2>&1', 'mount -o rw,remount /system')
foreach ($app in (Get-ChildItem $SRC -Directory)) {
    $where = if ($loc.ContainsKey($app.Name)) { $loc[$app.Name] } else { 'priv-app' }
    $dest  = "/system/$where/$($app.Name)"
    $lines += "rm -rf $dest/lib"
    # adb push into a NON-existent target puts the contents directly in it, with
    # no extra source-dir level - hence no pvr_applibs/ component here
    $lines += "cp -r /data/local/tmp/applibs/$($app.Name)/lib $dest/lib"
    $lines += "chown -R root:root $dest/lib"
    $lines += "find $dest/lib -type d -exec chmod 755 {} \;"
    $lines += "find $dest/lib -type f -exec chmod 644 {} \;"
}
# put the system SDK lib back the way it shipped
$lines += 'for a in lib64 lib; do'
$lines += '  if [ -f /system/$a/libPvr_UnitySDK.so.orig ]; then'
$lines += '    cp -f /system/$a/libPvr_UnitySDK.so.orig /system/$a/libPvr_UnitySDK.so'
$lines += '    rm -f /system/$a/libPvr_UnitySDK.so.orig'
$lines += '    echo "reverted /system/$a/libPvr_UnitySDK.so to stock"'
$lines += '  fi'
$lines += 'done'
$lines += 'sync'
$lines += 'mount -o ro,remount /system'
$lines += 'echo "--- app libs installed ---"'
$lines += 'for d in /system/priv-app/*/lib /system/app/*/lib; do'
$lines += '  [ -d "$d" ] && echo "$d: $(find $d -name "*.so" | wc -l) libs"'
$lines += 'done'

$sh = 'F:\PN2Lineage\tools\_applibs_dev.sh'
[System.IO.File]::WriteAllText($sh, ($lines -join "`n") + "`n")
& $adb push $sh /data/local/tmp/applibs.sh 2>&1 | Out-Null
& $adb shell "chmod 755 /data/local/tmp/applibs.sh; /system/xbin/su -c /data/local/tmp/applibs.sh" 2>&1

# Install the deodexed /oem apps into /system/priv-app.
#
# /oem itself is left untouched. Android 10 does not register anything from that
# partition on this build (stock 8.1 does), and rather than chase why, put them
# somewhere PackageManager definitely scans - which is also where they would live
# in an image we build ourselves.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$D   = 'PA7B40NGE5300009W'
$FIN = 'F:\PN2Lineage\oem_final'

& $adb -s $D shell rm -rf /data/local/tmp/oemapps 2>&1 | Out-Null
& $adb -s $D push $FIN /data/local/tmp/oemapps 2>&1 | Select-Object -Last 1

$lines = @('#!/system/bin/sh', 'exec 2>&1', 'mount -o rw,remount /system')
# NOT $d - PowerShell variables are case-insensitive, so $d would clobber $D
# (the device serial) and every later adb call would target a bogus device
foreach ($app in (Get-ChildItem $FIN -Directory)) {
    $n = $app.Name
    $lines += "rm -rf /system/priv-app/$n"
    $lines += "cp -r /data/local/tmp/oemapps/$n /system/priv-app/$n"
    $lines += "chown -R root:root /system/priv-app/$n"
    $lines += "find /system/priv-app/$n -type d -exec chmod 755 {} \;"
    $lines += "find /system/priv-app/$n -type f -exec chmod 644 {} \;"
}
$lines += 'sync'
$lines += 'mount -o ro,remount /system'
$lines += 'echo "--- installed ---"'
$lines += 'for n in PVRLauncher PVRHome store2d provision2d ToBToolService; do'
$lines += '  [ -d /system/priv-app/$n ] && echo "  $n: $(ls /system/priv-app/$n/*.apk 2>/dev/null | wc -l) apk"'
$lines += 'done'

$sh = 'F:\PN2Lineage\tools\_oeminstall_dev.sh'
[System.IO.File]::WriteAllText($sh, ($lines -join "`n") + "`n")
& $adb -s $D push $sh /data/local/tmp/oeminstall.sh 2>&1 | Out-Null
& $adb -s $D shell "chmod 755 /data/local/tmp/oeminstall.sh; /system/xbin/su -c /data/local/tmp/oeminstall.sh" 2>&1

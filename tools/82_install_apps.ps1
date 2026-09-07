# Install the re-signed Pico apps onto /system, preserving whether each lived in
# priv-app or app on stock (that distinction controls privileged permissions).
# adb writes its progress to stderr, which 'Stop' turns into a fatal error even
# on a successful push. Continue, and check results explicitly instead.
$ErrorActionPreference = 'Continue'
$adb  = 'C:\adb\adb.exe'
$SRC  = 'F:\PN2Lineage\pvr_apps'
$SGN  = 'F:\PN2Lineage\pvr_apps_signed'

# work out original location from the extraction tree
$loc = @{}
Get-ChildItem $SRC -Recurse -Filter '*.apk' | ForEach-Object {
    $loc[$_.Directory.Name] = $_.Directory.Parent.Name   # 'app' or 'priv-app'
}

$plan = @()
Get-ChildItem $SGN -Directory | ForEach-Object {
    $n   = $_.Name
    $apk = Get-ChildItem $_.FullName -Filter '*.apk' | Select-Object -First 1
    if (-not $apk) { return }
    $where = if ($loc.ContainsKey($n)) { $loc[$n] } else { 'priv-app' }
    $plan += [pscustomobject]@{ Name = $n; Apk = $apk.FullName; Dest = "/system/$where/$n" }
}

"=== plan ==="
$plan | ForEach-Object { "  {0,-22} -> {1}" -f $_.Name, $_.Dest }

# stage everything on the device first, then move into /system in one shot
& $adb shell rm -rf /data/local/tmp/pvrapps 2>&1 | Out-Null
& $adb shell mkdir -p /data/local/tmp/pvrapps 2>&1 | Out-Null
foreach ($p in $plan) {
    & $adb push $p.Apk "/data/local/tmp/pvrapps/$($p.Name).apk" 2>&1 | Out-Null
}
"`nstaged $($plan.Count) apks"

# build the on-device install script
$lines = @('#!/system/bin/sh', 'exec 2>&1', 'mount -o rw,remount /system')
foreach ($p in $plan) {
    $lines += "mkdir -p $($p.Dest)"
    $lines += "cp /data/local/tmp/pvrapps/$($p.Name).apk $($p.Dest)/$($p.Name).apk"
    $lines += "chmod 755 $($p.Dest)"
    $lines += "chmod 644 $($p.Dest)/$($p.Name).apk"
    $lines += "chown -R root:root $($p.Dest)"
}
# Android 10 refuses privileged apps whose privileged permissions are not in an
# allowlist XML. Pico shipped no such file (8.1 only warned). Downgrade the check
# to logging rather than hand-authoring allowlists for 9 apps we do not control.
$lines += 'grep -q "^ro.control_privapp_permissions" /system/build.prop || {'
$lines += '  echo "" >> /system/build.prop'
$lines += '  echo "# Pico apps ship no privapp-permissions allowlist (8.1 only warned)." >> /system/build.prop'
$lines += '  echo "ro.control_privapp_permissions=log" >> /system/build.prop'
$lines += '}'
$lines += 'sync'
$lines += 'mount -o ro,remount /system'
$lines += 'echo "--- installed ---"'
$lines += 'ls -d /system/priv-app/*/ /system/app/*/ 2>/dev/null | grep -iE "pvr|pico|vr|CVService|ShortcutMenu|InitServer|configserver"'
$lines += 'echo "privapp mode: $(getprop ro.control_privapp_permissions)"'

$sh = 'F:\PN2Lineage\tools\_install_apps_dev.sh'
[System.IO.File]::WriteAllText($sh, ($lines -join "`n") + "`n")
& $adb push $sh /data/local/tmp/install_apps.sh 2>&1 | Out-Null
"`n=== installing ==="
& $adb shell "chmod 755 /data/local/tmp/install_apps.sh; /system/xbin/su -c /data/local/tmp/install_apps.sh" 2>&1

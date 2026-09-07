# Copy the Pico persistent property set from the stock unit to the port.
#
# Stock has 102 pvr/pxr properties; the port has none. Pico's libraries branch on
# these - libpvrserviceclient never even calls dlopen for lib6DofReset.so, and
# logs "Open library<lib6DofReset.so> failed:(null)" with a null dlerror, i.e. a
# branch that was skipped rather than a load that failed.
#
# Only persist.* is copied. ro.* is baked into the build and init.svc.* is
# runtime state; neither is ours to set.
$ErrorActionPreference = 'Continue'
$adb   = 'C:\adb\adb.exe'
$STOCK = '192.168.0.139:5555'
$PORT  = 'PA7B40NGE5300009W'

$raw = & $adb -s $STOCK shell "getprop | grep -E 'persist\.(pvr|pxr|psmart)'" 2>&1
$props = @()
foreach ($line in $raw) {
    if ($line -match '^\[([^\]]+)\]:\s*\[(.*)\]\s*$') {
        $props += [pscustomobject]@{ Key = $Matches[1]; Val = $Matches[2] }
    }
}
"read $($props.Count) persistent Pico properties from stock"

# build an on-device script; setprop for each
$lines = @('#!/system/bin/sh', 'exec 2>&1', 'n=0')
foreach ($p in $props) {
    $v = $p.Val
    if ($v -eq '') { continue }   # don't set empties
    $lines += "setprop '$($p.Key)' '$v' && n=`$((n+1))"
}
$lines += 'echo "set $n properties"'
$lines += 'echo "--- verify a few ---"'
$lines += 'for k in persist.pvr.global_6dof persist.pvr.sdk.trackingmode persist.pvr.sensor.source persist.pvr.surface.rotation persist.pvr.config.eyebuffer_width; do'
$lines += '  echo "  $k = $(getprop $k)"'
$lines += 'done'

$f = 'F:\PN2Lineage\tools\_setprops_dev.sh'
[System.IO.File]::WriteAllText($f, ($lines -join "`n") + "`n")
& $adb -s $PORT push $f /data/local/tmp/setprops.sh 2>&1 | Out-Null
& $adb -s $PORT shell "chmod 755 /data/local/tmp/setprops.sh; /system/xbin/su -c /data/local/tmp/setprops.sh" 2>&1

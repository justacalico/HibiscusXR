# Two real gaps in the port, both fixed here.
#
# 1. /system/etc/public.libraries.txt came from the GSI, so it lost the 22 Pico
#    entries stock appends. On Android 8+ that file is what lets an app dlopen a
#    non-public /system library; without it every Pico VR library load from an app
#    namespace returns null.
#
# 2. Four libraries were never copied into our blob set at all:
#      libvirtualinputclient.so  libairclient.so  libSafetyArea.so  libImageGrid.so
#    libvirtualinputclient.so is the one that DEFINES pvrVirtualInputCreate (2 string
#    hits vs lib2dToVr.so's 1 reference), which is the exact null pointer VRShell
#    calls.
#
# Both are restorations of vendor configuration, not workarounds.
$adb   = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
$stage = 'F:\PN2Lineage\overlay_pvr'
$log   = 'F:\PN2Lineage\notes\188_publiclibs.log'
function L($m) { $s = "[{0}] {1}" -f (Get-Date -f 'HH:mm:ss'), $m; Add-Content $log $s; Write-Host $s }
Set-Content $log "=== public.libraries + missing Pico libs $(Get-Date) ==="

& $adb connect $STOCK 2>&1 | Out-Null
New-Item -ItemType Directory -Force "$stage\lib64" | Out-Null
New-Item -ItemType Directory -Force "$stage\lib"   | Out-Null

$missing = @('libvirtualinputclient.so','libairclient.so','libSafetyArea.so','libImageGrid.so')

L "--- pulling the 4 missing libs from the stock reference unit ---"
foreach ($l in $missing) {
  foreach ($d in @(@('lib64','lib64'), @('lib','lib'))) {
    $src = "/system/$($d[0])/$l"; $dst = "$stage\$($d[1])\$l"
    & $adb -s $STOCK shell "su -c 'cp $src /data/local/tmp/_p.so 2>/dev/null; chmod 644 /data/local/tmp/_p.so'" 2>&1 | Out-Null
    & $adb -s $STOCK pull /data/local/tmp/_p.so $dst 2>&1 | Out-Null
    if (Test-Path $dst) { L ("   pulled {0,-30} {1,-6} {2} bytes" -f $l, $d[0], (Get-Item $dst).Length) }
  }
}

L ""
L "--- building the merged public.libraries.txt ---"
$ours  = (& $adb -s $OURS  shell "cat /system/etc/public.libraries.txt" 2>&1) | ForEach-Object { "$_".Trim() }
$stk   = (& $adb -s $STOCK shell "cat /system/etc/public.libraries.txt" 2>&1) | ForEach-Object { "$_".Trim() }
$extra = $stk | Where-Object { $_ -and $_ -notmatch '^#' -and ($ours -notcontains $_) }
L ("   entries only on stock: " + $extra.Count)
$extra | ForEach-Object { L ("     + " + $_) }

$merged = @()
$merged += ($ours | Where-Object { $_ -ne '' })
$merged += ''
$merged += '# Pico Neo 2 VR stack. These come from the stock firmware; the GSI ships'
$merged += '# only the AOSP list, and without them an app namespace cannot dlopen any'
$merged += '# of the Pico libraries - dlsym returns null and the caller invokes it.'
$merged += $extra
Set-Content -Encoding ASCII "$stage\public.libraries.txt" (($merged -join "`n") + "`n")
L ("   wrote $stage\public.libraries.txt (" + $merged.Count + " lines)")

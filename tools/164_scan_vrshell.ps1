# Scan VRShell2's OWN bundled native libs for the x28 clobber.
#
# The system Pico libs are clean. But the JNI native method that the trampoline
# calls belongs to the app, and on 8.1 clobbering x28 costs nothing -- Android 10
# is the first version that parks the caller's sp there. So an ABI bug that has
# always been present in Pico's Unity plugin only becomes fatal on Q.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$dst = 'F:\PN2Lineage\notes\vrshell_lib'
New-Item -ItemType Directory -Force $dst | Out-Null

Write-Host "=== what VRShell2 ships ==="
$listing = (& $adb -s $DEV shell "su -c 'ls -l /system/priv-app/VRShell2/lib/arm64/'" 2>&1)
$listing | ForEach-Object { Write-Host ("  " + $_) }

# pull them all
$names = (& $adb -s $DEV shell "su -c 'ls /system/priv-app/VRShell2/lib/arm64/'" 2>&1) |
         ForEach-Object { "$_".Trim() } | Where-Object { $_ -like '*.so' }
foreach ($n in $names) {
  if (-not (Test-Path "$dst\$n")) {
    & $adb -s $DEV shell "su -c 'cp /system/priv-app/VRShell2/lib/arm64/$n /data/local/tmp/_l.so; chmod 644 /data/local/tmp/_l.so'" 2>&1 | Out-Null
    & $adb -s $DEV pull /data/local/tmp/_l.so "$dst\$n" 2>&1 | Out-Null
  }
}
Write-Host ""
Write-Host ("pulled " + (Get-ChildItem $dst -Filter *.so).Count + " libs")
Write-Host ""
Write-Host "=== x28 clobber scan (ALL functions) ==="
$out = 'F:\PN2Lineage\notes\164_vrshell_x28.txt'
Set-Content $out "=== VRShell2 x28 scan ==="
foreach ($f in Get-ChildItem $dst -Filter *.so) {
  Write-Host ("-- " + $f.Name + " (" + [math]::Round($f.Length/1MB,1) + " MB)")
  $r = wsl -e bash -c "~/.pn2venv/bin/python /mnt/f/PN2Lineage/tools/160_find_x28.py '/mnt/f/PN2Lineage/notes/vrshell_lib/$($f.Name)' --all --quiet" 2>&1
  if ($r) { $r | ForEach-Object { Add-Content $out "$_"; Write-Host "$_" } }
}
Add-Content $out "=== done ==="

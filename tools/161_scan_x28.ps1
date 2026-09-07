# Pull the 64-bit Pico libs and scan every exported JNI entry point for the
# x28-clobbering ABI violation that kills the generic JNI trampoline on Android 10.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$dst = 'F:\PN2Lineage\notes\lib64'
New-Item -ItemType Directory -Force $dst | Out-Null

$libs = @(
  'libPvr_UnitySDK.so','libPvr_UnitySDKExt1.so','libPvr_UnitySDKExt5.so',
  'libPvr_UnitySDKExt8.so','libPvr_UnitySDKExt9.so','libPvr_UnitySDKExt10.so',
  'libPvr_UnitySDKExt11.so','libPvr_UESDKExt2.so',
  'libpvrserviceclient.so','libpxrserviceclient.so','libplugin.pxr.so',
  'libruntime.pxr.so','libloader.pxr.so','libcompositor.pxr.so',
  'libjni_trackingfocus.so','libpvrmodule_platform.so',
  'libpvrmodule_orientationtracker.so','libconfigurationclient.pxr.so',
  'libCVControllerClient.pxr.so','libpxr_6dof_optimization.so'
)
foreach ($l in $libs) {
  if (-not (Test-Path "$dst\$l")) {
    & $adb -s $DEV pull "/system/lib64/$l" "$dst\$l" 2>&1 | Out-Null
  }
}
Write-Host ("pulled: " + (Get-ChildItem $dst -Filter *.so).Count + " libs")
Write-Host ""
Write-Host "=== scanning exported Java_* entry points for x28 clobbering ==="
$out = 'F:\PN2Lineage\notes\161_x28.txt'
Set-Content $out "=== x28 clobber scan (Java_* exports) ==="
foreach ($f in Get-ChildItem $dst -Filter *.so) {
  $r = wsl -e bash -c "~/.pn2venv/bin/python /mnt/f/PN2Lineage/tools/160_find_x28.py '/mnt/f/PN2Lineage/notes/lib64/$($f.Name)' --quiet" 2>&1
  if ($r) { $r | ForEach-Object { Add-Content $out $_; Write-Host $_ } }
}
Add-Content $out ""
Add-Content $out "=== done ==="

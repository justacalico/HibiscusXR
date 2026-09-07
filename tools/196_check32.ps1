# zygote AND zygote64 both preload public.libraries.txt, so the 32-bit copies must
# link too. The earlier closure check only covered lib64 - that omission is exactly
# the kind of thing that cost a bootloop, so verify the 32-bit set before applying.
$adb = 'C:\adb\adb.exe'
$DEV = 'PA7B40NGE5300009W'
$dst = 'F:\PN2Lineage\notes\lib32'
New-Item -ItemType Directory -Force $dst | Out-Null

$libs = @('libpvrserviceclient.so','libvirtualinputclient.so','libpxrserviceclient.so',
  'libairclient.so','libSafetyArea.so','libImageGrid.so','libPvr_UnitySDK.so',
  'libPvr_UnitySDKExt1.so','libPvr_UnitySDKExt5.so','libPvr_UnitySDKExt8.so',
  'libPvr_UnitySDKExt9.so','libPvr_UnitySDKExt10.so','libPvr_UnitySDKExt11.so',
  'libPvr_UESDKExt2.so','libCVControllerClient.pxr.so','lib6DofReset.so',
  'libpxrnotification.pxr.so','libconfigurationclient.pxr.so','libplugin.pxr.so',
  'libloader.pxr.so','libruntime.pxr.so','libcompositor.pxr.so')

foreach ($l in $libs) {
  if (Test-Path "$dst\$l") { continue }
  & $adb -s $DEV shell "su -c 'cp /system/lib/$l /data/local/tmp/_32.so 2>/dev/null; chmod 644 /data/local/tmp/_32.so'" 2>&1 | Out-Null
  & $adb -s $DEV pull /data/local/tmp/_32.so "$dst\$l" 2>&1 | Out-Null
}
Write-Host ("staged 32-bit: " + (Get-ChildItem $dst -Filter *.so).Count + " / " + $libs.Count)

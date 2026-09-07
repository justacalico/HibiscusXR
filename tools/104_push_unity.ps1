# Push the app-side Pico runtime libs into /system/lib{,64}.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$SRC = 'F:\PN2Lineage\pvr_stack'

$libs = @('libPvr_UnitySDK.so','libPvr_UnitySDKExt1.so','libPvr_UnitySDKExt5.so',
          'libPvr_UnitySDKExt8.so','libPvr_UnitySDKExt9.so','libPvr_UnitySDKExt10.so',
          'libPvr_UnitySDKExt11.so','libPvr_UESDKExt2.so','lib2dToVr.so',
          'libvraudio.so','libpicologkit.so')

& $adb shell rm -rf /data/local/tmp/unitylibs 2>&1 | Out-Null
& $adb shell mkdir -p /data/local/tmp/unitylibs/lib64 /data/local/tmp/unitylibs/lib 2>&1 | Out-Null

$n = 0
foreach ($arch in @('lib64','lib')) {
    foreach ($l in $libs) {
        $p = Join-Path $SRC "$arch\$l"
        if (Test-Path $p) {
            & $adb push $p "/data/local/tmp/unitylibs/$arch/$l" 2>&1 | Out-Null
            $n++
        }
    }
}
"pushed $n libs"

$sh = @'
#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp -f /data/local/tmp/unitylibs/lib64/*.so /system/lib64/ 2>/dev/null
cp -f /data/local/tmp/unitylibs/lib/*.so   /system/lib/   2>/dev/null
for f in /system/lib64/libPvr_*.so /system/lib/libPvr_*.so \
         /system/lib64/lib2dToVr.so /system/lib/lib2dToVr.so \
         /system/lib64/libvraudio.so /system/lib/libvraudio.so \
         /system/lib64/libpicologkit.so; do
  [ -f "$f" ] || continue
  chown root:root "$f"
  chmod 644 "$f"
done
sync
mount -o ro,remount /system
echo "--- installed ---"
ls -l /system/lib64/libPvr_UnitySDK.so /system/lib/libPvr_UnitySDK.so
echo "lib64 Pvr libs: $(ls /system/lib64/libPvr_*.so 2>/dev/null | wc -l)"
echo "lib   Pvr libs: $(ls /system/lib/libPvr_*.so 2>/dev/null | wc -l)"
'@
$f = 'F:\PN2Lineage\tools\_push_unity_dev.sh'
[System.IO.File]::WriteAllText($f, ($sh -replace "`r`n", "`n"))
& $adb push $f /data/local/tmp/push_unity.sh 2>&1 | Out-Null
& $adb shell "chmod 755 /data/local/tmp/push_unity.sh; /system/xbin/su -c /data/local/tmp/push_unity.sh" 2>&1

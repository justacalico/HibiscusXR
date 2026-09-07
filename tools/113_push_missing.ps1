# Push the libs the earlier hand-written extraction list missed.
# lib6DofReset.so is the one VRShell needs (updateOffsets, getResetPos); the rest
# are pulled because they matched the same Pico/sensor pattern and are cheap.
$ErrorActionPreference = 'Continue'
$adb = 'C:\adb\adb.exe'
$SRC = 'F:\PN2Lineage\pvr_stack'

$new = @('lib6DofReset.so','libjni_trackingfocus.so','libsensor_calibration.so',
         'libeye_tracking_dsp_sample_stub.so','libpico_factorytest3.so',
         'libpico_factorytest_jni.so','libpico_factorytest_sensor.so',
         'libpico_gyrocalibration.so','libpico_imucalibration.so',
         'libpico_node_connection.so')

& $adb shell rm -rf /data/local/tmp/newlibs 2>&1 | Out-Null
& $adb shell mkdir -p /data/local/tmp/newlibs/lib64 /data/local/tmp/newlibs/lib 2>&1 | Out-Null
$n = 0
foreach ($a in @('lib64','lib')) {
    foreach ($l in $new) {
        $p = Join-Path $SRC "$a\$l"
        if (Test-Path $p) { & $adb push $p "/data/local/tmp/newlibs/$a/$l" 2>&1 | Out-Null; $n++ }
    }
}
"pushed $n"

$sh = @'
#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp -f /data/local/tmp/newlibs/lib64/*.so /system/lib64/ 2>/dev/null
cp -f /data/local/tmp/newlibs/lib/*.so   /system/lib/   2>/dev/null
for f in /data/local/tmp/newlibs/lib64/*.so; do
  b=$(basename "$f"); [ -f /system/lib64/$b ] && chown root:root /system/lib64/$b && chmod 644 /system/lib64/$b
done
for f in /data/local/tmp/newlibs/lib/*.so; do
  b=$(basename "$f"); [ -f /system/lib/$b ] && chown root:root /system/lib/$b && chmod 644 /system/lib/$b
done
sync
mount -o ro,remount /system
ls -l /system/lib64/lib6DofReset.so /system/lib/lib6DofReset.so
'@
$f = 'F:\PN2Lineage\tools\_pushnew_dev.sh'
[System.IO.File]::WriteAllText($f, ($sh -replace "`r`n","`n"))
& $adb push $f /data/local/tmp/pushnew.sh 2>&1 | Out-Null
& $adb shell "chmod 755 /data/local/tmp/pushnew.sh; /system/xbin/su -c /data/local/tmp/pushnew.sh" 2>&1

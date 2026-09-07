# Does com.psmart.vrlib exist as a shared library on our image, and on stock?
#
# ALVR's jni.cpp does FindClass("com/psmart/vrlib/VrActivity") and hands the class
# to Pvr_SetInitActivity. Pico's own VR apps do the same thing internally. On
# Android that class comes from a framework jar exposed via a <library> entry in
# /system/etc/permissions -- if the permissions XML or the jar is missing, the app
# never gets the class, FindClass returns null, and the next JNI call faults.
$adb = 'C:\adb\adb.exe'
$OURS  = 'PA7B40NGE5300009W'
$STOCK = '192.168.0.139:5555'
$log = 'F:\PN2Lineage\notes\153_vrlib.log'
Set-Content $log "=== psmart/vrlib shared-library check ==="

$probe = @'
echo "--- framework jars mentioning psmart/pvr/vrlib ---"
ls /system/framework/ 2>/dev/null | grep -iE 'psmart|pvr|vrlib|pico'
echo "--- permissions XML declaring a psmart/pvr library ---"
grep -rl -iE 'psmart|vrlib|pvr' /system/etc/permissions/ 2>/dev/null
echo "--- the <library> lines themselves ---"
grep -rh -iE '<library[^>]*(psmart|vrlib|pvr)' /system/etc/permissions/ 2>/dev/null
echo "--- does any jar actually contain VrActivity ---"
for j in /system/framework/*.jar; do
  if unzip -l "$j" 2>/dev/null | grep -qi 'psmart/vrlib'; then echo "HIT $j"; fi
done
'@

foreach ($pair in @(@('OURS',$OURS), @('STOCK',$STOCK))) {
    $name = $pair[0]; $dev = $pair[1]
    Add-Content $log ""
    Add-Content $log "################ $name ($dev) ################"
    & $adb connect $dev 2>&1 | Out-Null
    $out = (& $adb -s $dev shell $probe 2>&1)
    $out | ForEach-Object { Add-Content $log "  $_" }
}
Get-Content $log

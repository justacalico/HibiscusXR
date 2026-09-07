#!/bin/bash
set -u
V=/home/justin/pn2/vendor
N=/mnt/f/PN2Lineage/notes

{
echo "################ /vendor/etc/qvr/6dof_config.xml ################"
cat "$V/etc/qvr/6dof_config.xml"
echo
echo "################ /vendor/etc/qvr/config_default.txt ################"
cat "$V/etc/qvr/config_default.txt"
echo
echo "################ /vendor/etc/qvr/qvrservice_config_default.txt ################"
grep -vE '^\s*#|^\s*$' "$V/etc/qvr/qvrservice_config_default.txt"
echo
echo "################ diff: eyetracking90 vs default ################"
diff "$V/etc/qvr/qvrservice_config_default.txt" "$V/etc/qvr/qvrservice_config_eyetracking90.txt"
echo
echo "################ /vendor/etc/qvr/svrapi_config_default.txt ################"
grep -vE '^\s*#|^\s*$' "$V/etc/qvr/svrapi_config_default.txt"
} > "$N/07a_qvr_configs.log" 2>&1

{
echo "################ init.picovr.rc : services and execs ################"
grep -nE '^\s*(service|exec|start|stop|on |setprop|write|chmod|chown|mount)' "$V/etc/pvr/init.picovr.rc" | head -220
echo
echo "################ init.picovr.rc : service blocks mentioning vr/track/cam ################"
awk '/^service /{blk=$0; inblk=1; next} inblk&&/^$/{inblk=0; blk=""} inblk{blk=blk"\n"$0} /^service /{}' "$V/etc/pvr/init.picovr.rc" >/dev/null
grep -A8 -iE '^service .*(qvr|pvr|pxr|vr|track|cam|sensor)' "$V/etc/pvr/init.picovr.rc"
} > "$N/07b_initpicovr.log" 2>&1

{
echo "################ pvr.display.fps.sh ################"
cat "$V/etc/pvr/pvr.display.fps.sh"
echo
echo "################ pvr.display.check.sh ################"
cat "$V/etc/pvr/pvr.display.check.sh"
echo
echo "################ pvr.vrdisplay.ui.sh ################"
cat "$V/etc/pvr/pvr.vrdisplay.ui.sh"
echo
echo "################ main.mci (hex+ascii) ################"
xxd "$V/etc/pvr/main.mci" | head -50
} > "$N/07c_display.log" 2>&1

{
echo "################ init.pvr.camera.calibration.sh ################"
cat "$V/etc/pvr/init.pvr.camera.calibration.sh"
echo
echo "################ pvr.calibration.qcom.sh (head 160) ################"
head -160 "$V/etc/pvr/pvr.calibration.qcom.sh"
} > "$N/07d_calibration.log" 2>&1

echo DONE
wc -l "$N"/07*.log

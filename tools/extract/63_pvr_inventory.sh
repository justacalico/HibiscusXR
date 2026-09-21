#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Inventory the Pico VR stack inside the STOCK system.img.
#
# /vendor kept the QVR/PVR *client* libraries, but the daemons, framework jars,
# permissions and apps all lived on the stock /system, which the GSI replaced.
# This is the list of what a compositor port has to bring back.
#
# debugfs, so the image is never mounted and never written.
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
OUT=${PN2_ROOT}/notes/63_pvr_inventory.txt
exec >"$OUT" 2>&1

echo "image: $IMG"
ls -l "$IMG"
echo

# figure out the layout: root-is-system, or a /system subdir
echo "=== root layout ==="
debugfs -R "ls -l /" "$IMG" 2>/dev/null | head -30
echo

# debugfs exits 0 even for a missing path, so probe by looking for real output
# rather than trusting the exit code.
ROOT=""
if debugfs -R "ls /system/bin" "$IMG" 2>/dev/null | grep -q '[a-z]'; then ROOT="/system"; fi
echo "using root prefix: '${ROOT}'  (empty means the image root IS /system)"
echo

echo "############ $ROOT/bin  (qvr/pvr/vr) ############"
debugfs -R "ls -l $ROOT/bin" "$IMG" 2>/dev/null | grep -iE 'qvr|pvr|pxr| vr|svr'
echo

echo "############ $ROOT/lib64 (vr libs) ############"
debugfs -R "ls -l $ROOT/lib64" "$IMG" 2>/dev/null | grep -iE 'qvr|pvr|pxr|svr|compositor|openxr|vrapi'
echo
echo "############ $ROOT/lib (vr libs) ############"
debugfs -R "ls -l $ROOT/lib" "$IMG" 2>/dev/null | grep -iE 'qvr|pvr|pxr|svr|compositor|openxr|vrapi'
echo

echo "############ $ROOT/framework (pico jars) ############"
debugfs -R "ls -l $ROOT/framework" "$IMG" 2>/dev/null | grep -iE 'pico|pvr|pxr|vr'
echo

echo "############ $ROOT/priv-app ############"
debugfs -R "ls -l $ROOT/priv-app" "$IMG" 2>/dev/null
echo

echo "############ $ROOT/app ############"
debugfs -R "ls -l $ROOT/app" "$IMG" 2>/dev/null
echo

echo "############ $ROOT/etc/permissions (pico) ############"
debugfs -R "ls -l $ROOT/etc/permissions" "$IMG" 2>/dev/null | grep -iE 'pico|pvr|pxr|vr|privapp'
echo

echo "############ $ROOT/etc/pvr  (the dangling psmvrapi symlink target) ############"
debugfs -R "ls -l $ROOT/etc/pvr" "$IMG" 2>/dev/null
echo

echo "############ $ROOT/etc/init (vr services) ############"
debugfs -R "ls -l $ROOT/etc/init" "$IMG" 2>/dev/null | grep -iE 'qvr|pvr|pxr|vr'
echo

echo "############ $ROOT/vendor symlink? / etc/vintf ############"
debugfs -R "ls -l $ROOT/etc/vintf" "$IMG" 2>/dev/null
echo DONE

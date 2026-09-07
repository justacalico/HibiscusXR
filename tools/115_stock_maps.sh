#!/system/bin/sh
# Run ON THE STOCK UNIT under su. Read-only.
# The complete library set a WORKING VRShell has mapped is the ground truth we
# have been reverse-engineering one crash at a time.
P=$(pidof com.pvr.vrshell)
echo "vrshell pid: $P"
echo
echo "##### all mapped .so #####"
grep -oE '/[^ ]*\.so' /proc/$P/maps | sort -u
echo
echo "##### 6dof / tracking / pxr / pvr #####"
grep -oE '/[^ ]*\.so' /proc/$P/maps | sort -u | grep -iE '6dof|reset|track|pxr|pvr|svr|psmart'
echo
echo "##### pvrservice mapped libs #####"
V=$(pidof pvrservice)
echo "pvrservice pid: $V"
grep -oE '/[^ ]*\.so' /proc/$V/maps | sort -u
echo
echo "##### does stock have lib6DofReset.so and where #####"
ls -l /system/lib64/lib6DofReset.so /system/lib/lib6DofReset.so 2>&1
echo
echo "##### full pvr property set #####"
getprop | grep -iE 'pvr|pxr|psmart|6dof'
echo DONE

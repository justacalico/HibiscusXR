#!/system/bin/sh
# ON THE STOCK UNIT. Restart VRShell with linker tracing so we capture a WORKING
# startup, then diff it against the port's failing one.
#
# VRShell is the home app, so ActivityManager relaunches it by itself. The
# headset blinks and comes back.
setprop debug.ld.app.com.pvr.vrshell dlopen,dlerror
logcat -c
am force-stop com.pvr.vrshell
sleep 2
# nudge it in case home does not auto-restart
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 8
echo "vrshell pid after restart: $(pidof com.pvr.vrshell)"
echo
echo "##### PvrServiceClient / library resolution #####"
logcat -d | grep -iE 'PvrServiceClient|Open library|Get function|VrApi|UnityNativeActivityPico' | head -60
echo
echo "##### every dlopen of a pico lib, in order #####"
logcat -d | grep -E 'dlopen\(name=' | grep -iE '6dof|pxr|pvr|svr|unity|main|il2cpp' | head -40
echo
echo "##### dlerrors #####"
logcat -d | grep -E 'dlerror set to' | head -20
echo DONE

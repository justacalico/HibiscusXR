# stock has global_6dof=true because its SLAM works; ours cannot calibrate, so
# take the 3DoF path instead of waiting on seethrough data forever
setprop persist.pvr.global_6dof false
setprop pvr.service.6dof.stopped true
setprop persist.pvr.sdk.trackingmode 1
setprop pvr.running.app.3dof true
setprop persist.pvrcon.seethrough.enable 0
setprop pvr.config.seethrough.enable 0
echo "--- props ---"
getprop persist.pvr.global_6dof; getprop persist.pvr.sdk.trackingmode; getprop pvr.running.app.3dof
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity

# match stock exactly - it reports 6dof stopped=false and global_6dof=true even
# though we cannot calibrate; the tracker still needs to be RUNNING to emit 3dof
setprop persist.pvr.global_6dof true
setprop pvr.service.6dof.stopped false
setprop persist.pvr.sdk.trackingmode 4
setprop pvr.running.app.3dof false
setprop pvr.display.vrmode.enable 1
am force-stop com.pvr.vrshell
sleep 2
stop pvrservice; sleep 2; start pvrservice; sleep 5
logcat -c
am start -n com.pvr.vrshell/.MainActivity

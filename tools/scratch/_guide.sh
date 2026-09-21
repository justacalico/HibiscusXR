setprop pvr.launcher.open.guide 0
setprop DEVICE_PROVISIONED 1
setprop pvr.display.vrmode.enable 1
setprop pvr.running.app.3dof false
setprop pvr.2d_screen.reposition 0
setprop pvr.controller.mode 1
echo "--- set ---"
getprop pvr.launcher.open.guide; getprop DEVICE_PROVISIONED; getprop pvr.display.vrmode.enable
am force-stop com.pvr.launcher
sleep 2
am start -n com.pvr.launcher/.MainActivity

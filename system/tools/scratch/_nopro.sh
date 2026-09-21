pm disable com.picovr.provision
setprop persist.pvrcon.seethrough.enable 0
am force-stop com.pvr.launcher
sleep 2
am start -n com.pvr.launcher/.MainActivity

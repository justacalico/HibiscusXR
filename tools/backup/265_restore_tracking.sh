#!/system/bin/sh
# My 3DoF experiment (global_6dof=false, trackingmode=1) froze the tracker: the
# reported rotation stopped changing and sat at identity. Stock runs
# global_6dof=true / trackingmode=4 even though its SLAM is what makes 6DoF work -
# the tracker has to be RUNNING to emit rotation at all. Put stock's values back.
setprop persist.pvr.global_6dof true
setprop pvr.service.6dof.stopped false
setprop persist.pvr.sdk.trackingmode 4
setprop pvr.running.app.3dof false
setprop pvr.display.vrmode.enable 1
stop pvrservice
sleep 2
start pvrservice
sleep 6
echo "=== is rotation live again? (values should differ line to line) ==="
logcat -c
sleep 4
logcat -d 2>/dev/null | grep 'getTrackingDataExt rotation' | tail -5
echo
echo "=== props ==="
getprop persist.pvr.global_6dof
getprop persist.pvr.sdk.trackingmode
getprop pvr.service.6dof.stopped

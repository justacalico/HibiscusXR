setprop persist.pvrcon.seethrough.enable 0
am force-stop com.pvr.seethrough.setting
sleep 2
am force-stop com.pvr.seethrough.setting
echo "  seethrough.enable = $(getprop persist.pvrcon.seethrough.enable)"
logcat -c
am start -n com.pvr.vrshell/.MainActivity 2>&1 | head -2
sleep 28
P=$(pidof com.pvr.vrshell)
echo "  vrshell pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"
dumpsys window | grep -m1 mCurrentFocus
echo "  BadPose=$(logcat -d | grep -c 'Bad Pose')  kLost=$(logcat -d | grep -c kLostDialog)"
echo "  seethrough respawns=$(logcat -d | grep -c 'com.pvr.seethrough')"
logcat -d | grep -iE "getTrackingDataExt position" | tail -2

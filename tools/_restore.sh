echo "--- stop the crash loop again ---"
pm disable com.picovr.picovrlib.cvcontroller 2>&1 | tail -1
am force-stop com.picovr.picovrlib.cvcontroller
am force-stop com.pvr.seethrough.setting
sleep 2
echo "--- clear the wedged pvrservice and its zombies ---"
stop pvrservice; sleep 3; start pvrservice; sleep 6
P=$(pidof pvrservice)
echo "  pvrservice pid $P  contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)  zombies=$(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P '$2==p' | grep -c Z)"
echo "--- back to the shell ---"
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
echo "  vrshell pid $(pidof com.pvr.vrshell)  threads=$(ls /proc/$(pidof com.pvr.vrshell)/task 2>/dev/null | wc -l)"
echo "--- tracking ---"
echo "  BadPose=$(logcat -d | grep -c 'Bad Pose')  kLost=$(logcat -d | grep -c kLostDialog)  segv=$(logcat -d | grep -c 'exited due to signal 11')"
logcat -d | grep -iE "getTrackingDataExt position" | tail -3

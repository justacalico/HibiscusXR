am start -n com.pvr.vrshell/.MainActivity 2>&1 | head -3
sleep 25
P=$(pidof com.pvr.vrshell)
echo "  vrshell pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"
echo "  focus: $(dumpsys window | grep -m1 mCurrentFocus)"
echo "  BadPose=$(logcat -d | grep -c 'Bad Pose')  kLost=$(logcat -d | grep -c kLostDialog)  segv=$(logcat -d | grep -c 'exited due to signal 11')"
echo "--- did it reach VR mode? ---"
logcat -d | grep -icE "EnterVrMode"
ls /proc/$P/task 2>/dev/null | while read t; do cat /proc/$P/task/$t/comm 2>/dev/null; done | grep -icE "warp"
echo "  ^ EnterVrMode count, then TimeWarp thread count"

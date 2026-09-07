P=$(pidof pvrservice)
echo "=== pvrservice wedged again? ==="
echo "  malloc/fork contended threads: $(debuggerd -b $P 2>&1 | grep -c 'je_malloc_mutex\|atfork')"
echo "  zombie am children: $(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P '$2==p' | grep -c Z)"
echo
echo "=== seethrough main thread ==="
S=$(pidof com.pvr.seethrough.setting)
echo "  pid $S"
debuggerd -b $S 2>&1 | grep -A6 "^\"com.pvr.seethrough" | head -8
echo
echo "=== CVService crash rate in the last buffer ==="
echo "  segv=$(logcat -d | grep -c 'exited due to signal 11')"
echo
echo "=== is the tracking camera stream back up? ==="
logcat -d | grep -iE "StartCameraInternal|StopCameraInternal|Created Thread VRTracker|Ending Thread VRTracker" | tail -6

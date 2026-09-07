P=$(pidof com.pvr.vrshell)
echo "pid=$P  threads=$(ls /proc/$P/task | wc -l)"
echo "=== main thread state ==="
cat /proc/$P/task/$P/stat 2>/dev/null | awk '{print "state:",$3}'
cat /proc/$P/task/$P/wchan 2>/dev/null; echo
echo
echo "=== native backtrace of all threads ==="
debuggerd -b $P 2>&1 | grep -E "^\"|#0[0-9]|Cmd line|native:" | head -60

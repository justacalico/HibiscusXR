#!/system/bin/sh
# Run airservice by hand with the same environment init gives it, so its own
# output is visible instead of just "stopped".
export LD_LIBRARY_PATH=/system/lib64/pvr_air:/system/lib64
export LD_PRELOAD=/system/lib64/pvr_air/libshim_air.so
echo "=== running airservice in the foreground for 6s ==="
/system/bin/airservice &
PID=$!
sleep 6
echo "--- still alive? ---"
if kill -0 $PID 2>/dev/null; then echo "  yes, pid $PID"; else echo "  no, it exited"; fi
echo "--- is it published? ---"
service list 2>/dev/null | grep -i air
kill $PID 2>/dev/null
echo
echo "=== its log output ==="
logcat -d -t 120 2>/dev/null | grep -iE 'airservice|AIRService|shim_air|aircamera|IAIRService' | tail -25

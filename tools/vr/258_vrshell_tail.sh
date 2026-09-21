#!/system/bin/sh
P=$(pidof com.pvr.vrshell)
echo "=== pid $P, threads $(ls /proc/$P/task 2>/dev/null | wc -l) ==="
echo "=== LAST 40 lines it logged ==="
logcat -d 2>/dev/null | grep " $P " | tail -40
echo
echo "=== thread names (is the unity/render thread there?) ==="
for t in /proc/$P/task/*; do cat $t/comm 2>/dev/null; done | sort | uniq -c | sort -rn | head -15

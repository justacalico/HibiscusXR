#!/system/bin/sh
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 14
P=$(pidof com.pvr.vrshell)
echo "=== pid $P, threads $(ls /proc/$P/task 2>/dev/null | wc -l) ==="
echo
echo "=== everything it logged ==="
logcat -d 2>/dev/null | grep " $P " | head -50
echo
echo "=== crashes / linker ==="
logcat -d -b crash 2>/dev/null | tail -15
logcat -d 2>/dev/null | grep -iE 'CANNOT LINK|UnsatisfiedLink|dlopen failed' | tail -8

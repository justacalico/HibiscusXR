#!/system/bin/sh
# qvrd is defined in vendor init and its binary exists, but it never starts.
# Start it by hand and capture why.
echo "=== before ==="
getprop init.svc.qvrd
echo "=== starting qvrd ==="
start qvrd
sleep 4
getprop init.svc.qvrd
ps -A 2>/dev/null | grep -i qvrservice
echo
echo "=== run it directly to see the real error ==="
/system/bin/qvrservice 2>&1 | head -10 &
sleep 3
kill %1 2>/dev/null
echo
echo "=== recent qvr / linker messages ==="
logcat -d -t 200 2>/dev/null | grep -iE 'qvr|CANNOT LINK|avc: denied.*qvr' | tail -20

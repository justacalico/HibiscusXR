#!/system/bin/sh
echo "=== full binder service list (pvr-ish) ==="
service list 2>/dev/null | grep -iE "pvr|picovr|vr_|_vr|display|sensor" | head -20
echo
echo "=== pvrservice process ==="
ps -A 2>/dev/null | grep -w pvrservice
echo
echo "=== does pvrservice open a unix socket instead of binder? ==="
P=$(pidof pvrservice)
ls -l /proc/$P/fd 2>/dev/null | grep -i socket | head -5
ls -l /dev/socket 2>/dev/null | grep -iE "pvr|qvr"
echo
echo "=== what the client would connect to ==="
strings -a /system/lib/libpvrserviceclient.so 2>/dev/null | grep -iE "^/dev/socket|pvr.*service|service.*pvr" | head -12

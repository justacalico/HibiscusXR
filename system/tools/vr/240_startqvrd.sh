#!/system/bin/sh
# Does init know qvrd at all? Its message when we ask distinguishes
# "service not defined" from "defined but failed to start".
echo "=== asking init to start qvrd ==="
setprop ctl.start qvrd
sleep 3
echo "init.svc.qvrd = $(getprop init.svc.qvrd)"
ps -A 2>/dev/null | grep -i qvrservice
echo
echo "=== init's own log for the attempt ==="
logcat -d -t 300 2>/dev/null | grep -iE ' init .*(qvrd|qvrservice)|service.*qvrd' | tail -12
echo
echo "=== is the binary usable by the system user? ==="
ls -lZ /system/bin/qvrservice

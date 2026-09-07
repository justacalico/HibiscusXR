#!/system/bin/sh
echo "=== is our rc present? ==="
ls -lZ /system/etc/init/pn2-qvrd.rc
echo "--- contents ---"
cat /system/etc/init/pn2-qvrd.rc
echo
echo "=== init parse errors / qvrd mentions in the boot log ==="
logcat -d -b main 2>/dev/null | grep -iE ' init ' | grep -iE 'qvrd|pn2-|parse|error|duplicate|invalid' | tail -25
echo
echo "=== dmesg (init logs there early) ==="
dmesg 2>/dev/null | grep -iE 'init:.*(qvrd|pn2-|duplicate|parse)' | tail -20
echo
echo "=== try starting it now ==="
setprop ctl.start qvrd
sleep 3
echo "init.svc.qvrd = $(getprop init.svc.qvrd)"
logcat -d -t 60 2>/dev/null | grep -iE 'init.*qvrd' | tail -10

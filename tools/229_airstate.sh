#!/system/bin/sh
echo "=== is airservice registered with servicemanager? ==="
service list 2>/dev/null | grep -i air
echo "=== processes ==="
ps -A 2>/dev/null | grep -iE 'airservice|virtual_input'
echo "=== init service state ==="
getprop | grep -iE 'init.svc.airservice|init.svc.virtual_input'
echo "=== any airservice/virtual_input errors in log ==="
logcat -d 2>/dev/null | grep -iE 'airservice|virtual_input|pvr_air|libskia|IAIRService' | tail -20

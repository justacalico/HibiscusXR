#!/system/bin/sh
# Hypothesis: airservice (class core, starts BEFORE pvrservice) wedges pvrservice,
# which then never registers with servicemanager, so every VR client hangs in
# getPvrService(). Stop airservice, restart pvrservice, and see if it registers.
echo "=== before ==="
service list 2>/dev/null | grep -i pvrservice || echo "  pvrservice NOT registered"
echo
echo "=== stopping airservice, restarting pvrservice ==="
stop airservice
sleep 2
stop pvrservice
sleep 2
start pvrservice
sleep 6
echo "=== after ==="
getprop init.svc.pvrservice
service list 2>/dev/null | grep -i pvrservice || echo "  pvrservice STILL not registered"
ps -A 2>/dev/null | grep -iE 'pvrservice|airservice'
echo
echo "=== pvrservice log since restart ==="
logcat -d -t 80 2>/dev/null | grep -i 'PvrService' | tail -12

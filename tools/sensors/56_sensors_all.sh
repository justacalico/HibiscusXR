#!/system/bin/sh
echo "=== sensors 0x19..end (the Pico private ones live up here) ==="
dumpsys sensorservice 2>/dev/null | grep -E '^0x[0-9a-f]+\)' | sed -n '25,60p'
echo
echo "=== every distinct type number in the list ==="
dumpsys sensorservice 2>/dev/null | grep -oE 'type: [^ ]+\([0-9]+\)' | sort -u
echo
echo "=== crash timestamps for the sensor abort ==="
logcat -d -b crash 2>/dev/null | grep -n 'DEVICE_PRIVATE_BASE' | head -20
echo
echo "=== is it still happening with nothing of ours running? ==="
echo "our app pid: $(pidof org.pn2.vrdemo)"
echo "system_server pid: $(pidof system_server)  uptime $(cut -d. -f1 /proc/uptime)s"
echo
echo "=== which sensors are ACTIVE right now ==="
dumpsys sensorservice 2>/dev/null | grep -E 'active-count' | head -10
echo DONE

#!/system/bin/sh
# Capture qvrservice's startup cleanly (no SIGTERM from us - last time "Plugin not
# valid" appeared only AFTER my own kill, so it may have been a shutdown message
# rather than the startup failure I took it for).
logcat -c
stop pn2_qvrd
sleep 2
start pn2_qvrd
sleep 10
echo "=== everything qvrservice logged at startup ==="
P=$(pidof qvrservice)
echo "pid $P  rss $(grep VmRSS /proc/$P/status 2>/dev/null | awk '{print $2}') kB"
logcat -d 2>/dev/null | grep -iE 'QVRService|qvrservice|QVRServicePlugin|QVRServiceCam|svr ' | head -50

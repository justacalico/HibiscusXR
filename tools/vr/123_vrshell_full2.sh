#!/system/bin/sh
# Full VRShell log. Works on both 8.1 (stock) and 10 (port):
#  - no awk (8.1 toybox does not have it), use grep on the pid column
#  - force-stop first so we always get a fresh start, and take the pid from
#    pidof rather than from a "Start proc" line that may not appear
logcat -c
am force-stop com.pvr.vrshell
sleep 2
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 3
PID=$(pidof com.pvr.vrshell)
sleep 5
echo "vrshell pid: $PID"
echo
if [ -n "$PID" ]; then
  logcat -d -v threadtime | grep -E "^[0-9-]+ [0-9:.]+ +$PID " | grep -v ' D linker' \
    | sed 's/^[0-9-]* [0-9:.]* *//'
else
  echo "(process already gone - dumping anything that mentions vrshell)"
  logcat -d -v threadtime | grep -iE 'vrshell|ConfigApi|psmvr|VrApi|VrServiceApi|PvrServiceClient|UnityNative' \
    | sed 's/^[0-9-]* [0-9:.]* *//'
fi
echo "##### END #####"

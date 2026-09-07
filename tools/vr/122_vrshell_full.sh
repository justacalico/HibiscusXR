#!/system/bin/sh
# Full VRShell log, linker noise stripped, so it can be diffed line for line
# against the stock capture. Linker tracing off to keep it readable.
setprop debug.ld.app.com.pvr.vrshell 0
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 7
PID=$(logcat -d | grep -oE 'Start proc [0-9]+:com\.pvr\.vrshell' | head -1 | grep -oE '[0-9]+')
echo "vrshell pid: $PID"
echo
logcat -d -v threadtime | awk -v p="$PID" '$3==p' | grep -v ' D linker' \
  | sed 's/^[0-9-]* [0-9:.]* *//'
echo "##### END #####"

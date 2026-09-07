#!/system/bin/sh
# Launch VRShell and dump ONLY its process's log lines, so other Pico apps
# (notably 32-bit ALVR, which also uses the Pico SDK) do not pollute the diff.
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 7
# find the pid ActivityManager assigned, even though the process is gone by now
PID=$(logcat -d | grep -oE 'Start proc [0-9]+:com\.pvr\.vrshell' | head -1 | grep -oE '[0-9]+')
echo "vrshell pid was: $PID"
echo
echo "##### everything that pid logged #####"
logcat -d -v threadtime | awk -v p="$PID" '$3==p' | sed 's/^[0-9-]* [0-9:.]* *//' | head -60
echo DONE

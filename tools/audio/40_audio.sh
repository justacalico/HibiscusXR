#!/system/bin/sh
echo "=== audioserver service state ==="
getprop init.svc.audioserver
getprop init.svc.vendor.audio-hal-2-0
getprop init.svc.audio-hal-2-0
echo "pid: $(pidof audioserver)"
echo "uptime: $(cut -d. -f1 /proc/uptime)s"
echo

echo "=== audio-related processes ==="
ps -A -o PID,NAME 2>/dev/null | grep -iE 'audio|media'
echo

echo "=== audio HAL registered? ==="
lshal 2>/dev/null | grep -i audio
echo

echo "=== declared in vendor manifest? ==="
grep -B2 -A8 'hardware\.audio' /vendor/manifest.xml 2>/dev/null | head -60
echo

echo "=== audio HAL binaries in vendor ==="
ls -l /vendor/bin/hw/ 2>/dev/null | grep -i audio
echo

echo "=== audioserver crash / restart evidence ==="
logcat -d -b crash 2>/dev/null | grep -iA12 'audioserver' | head -50
echo "---- init restarts ----"
logcat -d 2>/dev/null | grep -iE 'audioserver|audio-hal|audio@' | head -40
echo

echo "=== tombstones ==="
ls -lt /data/tombstones/ 2>/dev/null | head -6
echo

echo "=== any HAL still declared-but-missing (the same trap as boot) ==="
for h in $(grep -o '<name>[a-z0-9.]*hardware[a-z0-9.]*</name>' /vendor/manifest.xml 2>/dev/null | sed 's/<[^>]*>//g' | sort -u); do
  reg=$(lshal 2>/dev/null | grep -c "$h")
  if [ "$reg" = "0" ]; then echo "NOT REGISTERED: $h"; fi
done
echo DONE

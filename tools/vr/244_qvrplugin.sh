#!/system/bin/sh
# Ours: qvrservice VSZ ~15 MB and "Plugin not valid".
# Stock: VSZ ~187 MB and real camera clients.
# So qvrservice runs but never loads its plugin. Find which plugin and why.
echo "=== qvrservice own log ==="
logcat -d 2>/dev/null | grep -iE 'QVRService|QVRServicePlugin|qvrservice:' | tail -25
echo
echo "=== qvr plugin libraries ==="
ls -l /vendor/lib64/ 2>/dev/null | grep -iE 'qvr'
ls -l /vendor/lib/ 2>/dev/null | grep -iE 'qvr'
echo
echo "=== config naming the plugin ==="
for f in /vendor/etc/qvr* /vendor/etc/*qvr* /etc/qvr* /vendor/etc/vr/* ; do
  [ -f "$f" ] && echo "--- $f ---" && cat "$f"
done 2>/dev/null | head -60
echo
echo "=== what qvrservice has mapped (plugins loaded?) ==="
P=$(pidof qvrservice)
echo "pid $P"
grep -oE '/[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u | head -30

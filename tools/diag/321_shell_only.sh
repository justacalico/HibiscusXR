#!/system/bin/sh
# Isolate VRShell's own lines. Other 32-bit processes (CVService et al) init the
# SDK fine, so the question is specifically what the shell does before stalling.
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 20
V=$(pidof com.pvr.vrshell)
echo "=== vrshell pid $V ==="
echo
echo "=== ONLY its lines, first 45 ==="
logcat -d 2>/dev/null | grep " $V " | grep -viE 'mBitmap|chatty' | head -45
echo
echo "=== thread names (is UnityMain there?) ==="
for t in /proc/$V/task/*; do cat $t/comm 2>/dev/null; done | sort -u | tr '\n' ' '
echo

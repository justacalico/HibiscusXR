#!/system/bin/sh
# qvrservice is the root blocker: no QVR client -> supportedTrackingModes = 0 ->
# trackingstate 0 -> pose rejected -> black screen. Ours logs "Plugin not valid"
# and is ~15MB resident vs stock's ~187MB. Find what stock loads that we do not.
P=$(pidof qvrservice)
echo "=== qvrservice pid $P ==="
[ -z "$P" ] && exit 0
echo "rss: $(grep VmRSS /proc/$P/status 2>/dev/null)"
echo
echo "=== shared libraries it has mapped ==="
grep -oE '/[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo
echo "=== open fds (camera/ion/dsp?) ==="
ls -l /proc/$P/fd 2>/dev/null | grep -oE '/dev/[a-z0-9_/]*' | sort | uniq -c | head -15
echo
echo "=== plugin-related strings in the binary ==="
strings -a /system/bin/qvrservice 2>/dev/null | grep -iE 'plugin|\.so' | sort -u | head -20

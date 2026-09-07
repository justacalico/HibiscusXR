#!/system/bin/sh
# What is the see-through app doing, and why does cvcontroller crash-loop?
P=$(pidof com.pvr.seethrough.setting)
echo "=== seethrough pid: $P ==="
echo "--- its own log lines ---"
logcat -d | grep " $P " | tail -30
echo
echo "=== newest tombstone ==="
T=$(ls -t /data/tombstones/tombstone_* 2>/dev/null | head -1)
echo "file: $T"
grep -E "^pid:|^signal|Cause:" "$T"
grep -E "#0[0-8] pc" "$T"

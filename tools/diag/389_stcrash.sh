#!/system/bin/sh
echo "=== the see-through crash, full ==="
logcat -d | grep -A30 'E CRASH.*com.pvr.seethrough' | head -40

echo
echo "=== unity CRASH tag block ==="
logcat -d | grep -E ' E CRASH' | head -30

echo
echo "=== matching tombstone ==="
logcat -d | grep -B4 -A26 'fault addr 0000000000000010' | head -45

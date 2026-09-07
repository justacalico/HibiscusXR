#!/system/bin/sh
# See-through starts, the SafetyArea algorithm inits, one camera frame arrives,
# then the app exits. Find out why, and whether the missing GSA_AlgConfig.yml
# matters.
echo "=== does the algorithm config exist anywhere? ==="
ls -l /sdcard/GSA_AlgConfig.yml 2>/dev/null || echo "  /sdcard/GSA_AlgConfig.yml ABSENT"
find /system /vendor /sdcard /data -name "GSA_AlgConfig.yml" 2>/dev/null | head
find /system /vendor -name "*.yml" 2>/dev/null | head -10

echo
echo "=== how did the app die? ==="
logcat -d | grep -iE 'seethrough' | grep -iE 'died|kill|ANR|force|crash|exit|Process' | tail -12

echo
echo "=== ActivityManager view ==="
logcat -d | grep -iE 'ActivityManager|ActivityTaskManager' | grep -i seethrough | tail -10

echo
echo "=== fatal / fdsan / tombstones ==="
logcat -d | grep -E 'FATAL|signal 3[0-9]|signal 11|fdsan' | tail -8

echo
echo "=== last lines the app logged before going ==="
logcat -d | grep -iE 'seethrough|SafetyArea|AIRClient' | tail -25

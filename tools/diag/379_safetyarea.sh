#!/system/bin/sh
# Third instance of the same Treble trap. libSafetyAreaRecovery.so ships only in
# /vendor/lib64; airservice runs from /system/bin and Android 10 forbids a /system
# process loading from /vendor/lib. So the see-through / boundary algorithm
# (AlgorithmManager type 3) fails its dlopen and loadSymbols, and the calibration
# can never run - which is why stdata.txt here is 0 bytes and stock's is 21813.
set -u
mount -o rw,remount /system

echo "=== deps of the vendor copy (must all resolve from /system) ==="
strings -a /vendor/lib64/libSafetyAreaRecovery.so 2>/dev/null | grep -E '^lib.*\.so$' | sort -u | while read l; do
  if [ -f "/system/lib64/$l" ] || [ -f "/apex/com.android.runtime/lib64/$l" ] || [ -f "/system/lib64/pvr_air/$l" ]; then
    echo "  ok      $l"
  else
    echo "  MISSING $l"
  fi
done

echo
echo "=== install ==="
cp -f /vendor/lib64/libSafetyAreaRecovery.so /system/lib64/libSafetyAreaRecovery.so
chmod 644 /system/lib64/libSafetyAreaRecovery.so
chown root:root /system/lib64/libSafetyAreaRecovery.so
chcon u:object_r:system_lib_file:s0 /system/lib64/libSafetyAreaRecovery.so 2>/dev/null
sync
ls -l /system/lib64/libSafetyAreaRecovery.so

echo
echo "=== restart airservice so it picks it up ==="
stop airservice; sleep 2; start airservice; sleep 8
echo "  airservice pid $(pidof airservice)"

echo
echo "=== relaunch see-through ==="
logcat -c
am force-stop com.pvr.seethrough.setting
sleep 2
am start -n com.pvr.seethrough.setting/.MainActivity >/dev/null 2>&1
sleep 30

S=$(pidof com.pvr.seethrough.setting)
echo "  seethrough pid [$S] threads=$(ls /proc/$S/task 2>/dev/null | wc -l)"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== did the algorithm load this time? ==="
logcat -d | grep -iE 'SafetyAreaRecovery|startAlgorithm|loadSymbols|AlgorithmManager' | tail -10

echo
echo "=== camera feed ==="
logcat -d | grep -iE 'AIRService|aircamera|processCameraStateChange' | grep -v LockBuffer | tail -12

echo
echo "=== boundary data written? ==="
ls -l /data/misc/user/0/boundary/stdata.txt /data/misc/user/0/boundary/stdataforalgorithm.txt

echo
echo "  segv=$(logcat -d | grep -c 'exited due to signal 11')  BadPose=$(logcat -d | grep -c 'Bad Pose')  kLost=$(logcat -d | grep -c kLostDialog)"

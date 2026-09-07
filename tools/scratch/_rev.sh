T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so
mount -o rw,remount /system
cp /data/local/tmp/p2.so "$T.new"
chmod 644 "$T.new"; chown root:root "$T.new"
chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null
mv -f "$T.new" "$T"
sync
md5sum "$T" /data/local/tmp/p2.so
echo "--- controller service stays disabled for now ---"
pm list packages -d | grep cvcontroller
stop pvrservice; sleep 3; start pvrservice; sleep 6
echo "--- launch ---"
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 30
P=$(pidof com.pvr.vrshell)
echo "  pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"
echo "--- crashes? ---"
logcat -d | grep -E "E CRASH|signal 11" | head -6
echo "  (empty = no crash)"
echo "--- vr mode + compositor ---"
logcat -d | grep -iE "EnterVrMode|TimeWarp|SubmitFrame|frame rate|Compositor" | tail -14
echo "--- tracking ---"
BP=$(logcat -d | grep -c "Bad Pose"); KL=$(logcat -d | grep -c kLostDialog); TS=$(logcat -d | grep -c "trackingstate = 0x0")
echo "  BadPose=$BP kLost=$KL ts0=$TS"

T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so
mount -o rw,remount /system
cp /data/local/tmp/x24.so "$T.new"
chmod 644 "$T.new"; chown root:root "$T.new"
chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null
mv -f "$T.new" "$T"
sync
md5sum "$T" /data/local/tmp/x24.so
echo "--- launching ---"
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
echo "  pid $(pidof com.pvr.vrshell)"
echo "--- did pvrservice finally ask QVR? ---"
logcat -d | grep -iE "QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create" | tail -5
echo "--- does pvrservice load the qvr client now? ---"
grep -oE "libqvr[^ ]*\.so" /proc/$(pidof pvrservice)/maps 2>/dev/null | sort -u
echo "--- compositor ---"
echo "  BadPose  = $(logcat -d | grep -c \"Bad Pose\")"
echo "  kLost    = $(logcat -d | grep -c kLostDialog)"
echo "  ts0      = $(logcat -d | grep -c \"trackingstate = 0x0\")"
echo "--- hmdInfo (did EnterVrMode read sane values?) ---"
logcat -d | grep -iE "hmdInfo|EnterVrMode" | head -8

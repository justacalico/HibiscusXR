mount -o rw,remount /system
# check what it needs before installing
echo "--- deps of the vendor copy ---"
cp -f /vendor/lib64/libqvrservice_client.so /system/lib64/libqvrservice_client.so
cp -f /vendor/lib/libqvrservice_client.so   /system/lib/libqvrservice_client.so
# the camera client is the same story - pvrservice/airservice need it too
cp -f /vendor/lib64/libqvrcamera_client.so  /system/lib64/libqvrcamera_client.so 2>/dev/null
cp -f /vendor/lib/libqvrcamera_client.so    /system/lib/libqvrcamera_client.so 2>/dev/null
chmod 644 /system/lib64/libqvr*.so /system/lib/libqvr*.so
chown root:root /system/lib64/libqvr*.so /system/lib/libqvr*.so
chcon u:object_r:system_lib_file:s0 /system/lib64/libqvr*.so /system/lib/libqvr*.so 2>/dev/null
sync
ls -l /system/lib64/libqvrservice_client.so /system/lib64/libqvrcamera_client.so
echo "--- restart pvrservice so it re-asks QVR ---"
stop pvrservice; sleep 2; start pvrservice; sleep 6
echo "--- launch the shell ---"
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
echo "  pid $(pidof com.pvr.vrshell)"
echo "--- QVR verdict now ---"
logcat -d | grep -iE "QVR Serivce reported|QVR Service supports|Calling QVRServiceClient_Create" | tail -6
echo "--- does pvrservice map the client now? ---"
grep -oE "libqvr[^ ]*\.so" /proc/$(pidof pvrservice)/maps 2>/dev/null | sort -u
echo "--- compositor ---"
BP=$(logcat -d | grep -c "Bad Pose"); KL=$(logcat -d | grep -c kLostDialog); TS=$(logcat -d | grep -c "trackingstate = 0x0")
echo "  BadPose=$BP  kLost=$KL  ts0=$TS"

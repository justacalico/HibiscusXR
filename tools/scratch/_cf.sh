echo "--- who else is holding QVR / cameras ---"
ps -A | grep -iE "airservice|qvrservicetest|seethrough" | awk "{print \"  \"\$1\" \"\$NF}"
echo "--- stop the competition ---"
stop airservice 2>/dev/null; pkill -f qvrservicetest 2>/dev/null
am force-stop com.pvr.vrshell
sleep 3
echo "--- restart pvrservice so the tracker starts clean ---"
stop pvrservice; sleep 3; start pvrservice; sleep 8
echo "  pvrservice pid $(pidof pvrservice)"
logcat -c
echo "--- launch the shell ---"
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
echo "--- watch tracking for 60s ---"
i=0
while [ $i -lt 6 ]; do
  sleep 10
  BP=$(logcat -d | grep -c "Bad Pose")
  TR=$(logcat -d | grep -c "Ending Thread VRTracker")
  CS=$(logcat -d | grep -c "QVR_CAM_DEVICE_STOPPING")
  echo "  t+$(( (i+1)*10 ))s  BadPose=$BP  trackerExit=$TR  camStopping=$CS"
  i=$((i+1))
done
echo "--- tracker thread history ---"
logcat -d | grep -iE "VRTracker|6DOF|QVR_CAM|Starting Thread|Ending Thread" | tail -14
echo "--- who opened the cameras ---"
logcat -d | grep -iE "QVRServiceCamDeviceHAL3|camera.*open|CamDevice" | tail -8

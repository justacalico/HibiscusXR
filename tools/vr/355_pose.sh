#!/system/bin/sh
# Cameras stream and the tracker thread is alive, but TimeWarp rejects the pose
# orientation. Find out what the tracking state and pose values actually are.
logcat -c
sleep 6
echo "=== QVR tracker state transitions ==="
logcat -d | grep -iE "QVRServiceTracker|tracking state|TrackingState|QVR_TRACKING|state=" | grep -viE "LockBuffer" | tail -15
echo
echo "=== pvrservice pose / 6dof output ==="
logcat -d | grep -iE "PvrService|PvrPlatform" | grep -iE "pose|quat|orientation|position|6dof|predict" | tail -15
echo
echo "=== the SDK's cached tracking state ==="
logcat -d | grep -iE "trackingstate|GetTrackingDataExt|CalculateDialogState|BoundarySystem" | tail -10
echo
echo "=== TimeWarp rejection detail - is bufferNum advancing or frozen? ==="
logcat -d | grep "Bad Pose" | tail -6
logcat -d | grep -o "bufferNum [0-9]*" | sort -u | tail -8
echo
echo "=== is the IMU still delivering? ==="
logcat -d | grep -iE "gyro|accel|SensorDataFusion|SensorFusion|imu" | tail -8
echo
echo "=== sensor nodes present ==="
ls /dev/input/ 2>/dev/null | head
getprop | grep -iE "6dof|trackingmode|global_6dof" | head

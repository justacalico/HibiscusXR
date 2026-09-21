echo "=== pose quality / svr right now ==="
logcat -d | grep -E " svr *:" | tail -8
echo
echo "=== trackingstate values seen ==="
logcat -d | grep -oE "trackingstate = 0x[0-9a-f]+,0x[0-9a-f]+" | sort | uniq -c | tail -6
echo
echo "=== bad pose vs total in the buffer ==="
echo "  BadPose  = $(logcat -d | grep -c 'Bad Pose')"
echo "  kLost    = $(logcat -d | grep -c kLostDialog)"
echo
echo "=== live pose ==="
logcat -d | grep -iE "svrHeadPoseStateOutput|getTrackingDataExt position" | tail -4
echo
echo "=== what is focused ==="
dumpsys window | grep mCurrentFocus

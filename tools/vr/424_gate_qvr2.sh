#!/system/bin/sh
# Round 2. Last run started VR mode standalone but produced pose_quality=0 with an
# identity quat, so the 6DoF algorithm never engaged. Two suspects:
#   - the test binary never got the 6DOF XML pushed (-x), which pvrservice does do
#   - I tail'd the output and threw away the setup phase where the mode is set
# Capture everything this time.
set -u
LOG=/data/local/tmp/gate2.log

echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)   qvrservice = $(pidof qvrservice 2>/dev/null || echo DEAD)"

echo
echo "########## supported tracking modes, straight from the service ##########"
/vendor/bin/qvrservicetest64 -u P 2>&1 | grep -iE "tracking-mode|supported|6dof|mode" | head -20

echo
echo "########## run: -t 6 with the 6DOF XML explicitly ##########"
logcat -c 2>/dev/null
/vendor/bin/qvrservicetest64 -t 6 -d 30 -p 1000000 -x /vendor/etc/qvr/6dof_config.xml -l P > $LOG 2>&1
echo "  --- SETUP PHASE (first 25 lines) ---"
head -25 $LOG
echo "  --- LAST POSE ---"
grep -A3 "Head tracking pose" $LOG | tail -8
echo "  --- pose quality over the run ---"
grep -o "pose_quality = [0-9.]*" $LOG | sort | uniq -c
echo "  --- tracking_state values seen ---"
grep -o "tracking_state=[0-9]*" $LOG | sort | uniq -c
grep -o "tracking_warning_flags=[0-9]*" $LOG | sort | uniq -c

echo
echo "########## did the tracking camera actually stream? ##########"
logcat -d 2>/dev/null | grep -iE "CamDevice|camera|frame|exposure" | tail -15

echo
echo "########## 6DOF / IMU health unit test ##########"
/vendor/bin/qvrservicetest64 -u GI -d 15 2>&1 | tail -25

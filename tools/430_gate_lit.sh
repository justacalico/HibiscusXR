#!/system/bin/sh
# Completes gate 1. The dark test proved the QVR pose path works standalone via
# 3DoF. This proves the visual half: camera_quality and pose_quality climbing,
# and a real non-zero position, with nothing of pico's running.
set -u

echo "########## A. baseline: does tracking work AT ALL right now ##########"
echo "  (full pico stack up, so this is the known-good control)"
for p in qvrservice pvrservice airservice; do echo "  $p = $(pidof $p 2>/dev/null || echo -)"; done
logcat -c 2>/dev/null
sleep 6
logcat -d 2>/dev/null | grep -iE "pose_quality|camera_quality|BadPose|kLost|tracking_state" | tail -6
echo "  (if this is empty pvrservice just is not logging quality, not a failure)"

echo
echo "########## B. release VR mode ##########"
stop pvrservice 2>/dev/null; stop airservice 2>/dev/null
sleep 2
for p in pvrservice airservice; do
  pid=$(pidof $p 2>/dev/null); [ -n "$pid" ] && kill -9 $pid 2>/dev/null
done
sleep 2
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"
echo "  qvrservice = $(pidof qvrservice 2>/dev/null || echo DEAD)"

echo
echo "########## C. 6DoF standalone, 60 s, sample every 2 s ##########"
logcat -c 2>/dev/null
/vendor/bin/qvrservicetest64 -t 6 -d 60 -p 2000000 -l P > /data/local/tmp/lit6.log 2>&1

echo "  --- setup ---"
grep -iE "setting tracking mode|starting VR|Api version|ERROR|Failed" /data/local/tmp/lit6.log | head -8

echo "  --- quality progression (this is the answer) ---"
grep -o "pose_quality = [0-9.]* sensor_quality=[0-9.]* camera_quality=[0-9.]*" /data/local/tmp/lit6.log | cat -n | head -35

echo "  --- did position ever leave the origin? ---"
grep -o "position=([^)]*)" /data/local/tmp/lit6.log | sort -u | head -12

echo "  --- tracking_state distribution ---"
grep -o "tracking_state=[0-9]*" /data/local/tmp/lit6.log | sort | uniq -c

echo
echo "########## D. tracker + camera side ##########"
logcat -d 2>/dev/null | grep -iE "QVRServiceTracker|6DOF|map|camera_quality|CamDevice" | tail -15

echo
echo "########## E. restore ##########"
start pvrservice 2>/dev/null; start airservice 2>/dev/null
sleep 4
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"
echo "  vrshell    = $(pidof com.pvr.vrshell 2>/dev/null || echo DEAD)"
echo "  fan rpm    = $(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"

#!/system/bin/sh
# Dark-room-valid gate. 6DoF is visual-inertial so pose_quality is meaningless
# without light. 3DoF is gyro+accel only, so it proves the QVR pose path end to
# end with no camera dependency. Only pico service touched is pvrservice/airservice,
# and only because QVR allows exactly one VR-mode owner.
set -u

echo "########## baseline ##########"
echo "  uptime $(cut -d. -f1 /proc/uptime)s"
for p in qvrservice pvrservice airservice adsprpcd cdsprpcd; do
  echo "  $p = $(pidof $p 2>/dev/null || echo -)"
done

echo
echo "########## release VR mode: stop ONLY the two pico daemons ##########"
stop pvrservice 2>/dev/null; stop airservice 2>/dev/null
sleep 2
for p in pvrservice airservice; do
  pid=$(pidof $p 2>/dev/null); [ -n "$pid" ] && kill -9 $pid 2>/dev/null
done
sleep 2
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"
echo "  qvrservice = $(pidof qvrservice 2>/dev/null || echo DEAD)  <- must survive"

echo
echo "########## TEST A: 3DoF pose, camera-independent ##########"
echo "  expect a non-identity quaternion if the QVR pose path works standalone"
logcat -c 2>/dev/null
/vendor/bin/qvrservicetest64 -t 3 -d 12 -p 2000000 -l P > /data/local/tmp/dark3.log 2>&1
echo "  --- setup ---"
grep -iE "setting tracking mode|starting VR|Api version|ERROR|Failed|Invalid" /data/local/tmp/dark3.log | head -10
echo "  --- quaternions seen ---"
grep -o "quat =  ([^)]*)" /data/local/tmp/dark3.log | sort -u | head -8
echo "  --- qualities ---"
grep -o "sensor_quality=[0-9.]*" /data/local/tmp/dark3.log | sort | uniq -c
grep -o "tracking_state=[0-9]*" /data/local/tmp/dark3.log | sort | uniq -c

echo
echo "########## TEST B: does the tracking camera deliver frames at all ##########"
timeout 25 /vendor/bin/qvrcameratest64 -c tracking -n 10 2>&1 | tail -20 || \
  /vendor/bin/qvrcameratest64 2>&1 | head -20

echo
echo "########## TEST C: did the 6dof DSP skel actually load ##########"
logcat -d 2>/dev/null | grep -iE "tracker_6dof|VIOMapping|dsp_driver|skel|fastrpc|adsp" | tail -12

echo
echo "########## restore ##########"
start pvrservice 2>/dev/null; start airservice 2>/dev/null
sleep 3
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"

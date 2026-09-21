#!/system/bin/sh
# Decisive question: does qvrservice run the 6DoF itself, or does pvrservice drive it?
# I claimed the former. This test has to be able to prove me wrong.
set -u

echo "########## what tracking_state=3 actually means ##########"
strings -a /vendor/bin/qvrservicetest64 | grep -iE "TRACKING_STATE|STOPPED|INITIALIZING|RELOCAL|FATAL|INVALID" | head -20
strings -a /system/lib64/libqvrservice_client.so | grep -iE "QVRSERVICE_TRACKING_STATE|TRACKING_STATE_" | head -20

echo
echo "########## which DSP algorithms are actually installed ##########"
ls /vendor/lib/rfsa/adsp/ 2>/dev/null
echo "--- any 6dof/slam/tracking skel? ---"
ls /vendor/lib/rfsa/adsp/ 2>/dev/null | grep -iE "6dof|slam|track|vr|xr|cv"

echo
echo "########## does pvrservice ship its own SLAM? ##########"
for f in /system/lib/libpvr*.so /system/lib64/libpvr*.so; do
  [ -e "$f" ] || continue
  n=$(strings -a "$f" 2>/dev/null | grep -icE "orbslam|ORBvoc|slam|relocal" 2>/dev/null)
  [ "$n" -gt 0 ] 2>/dev/null && echo "  $f  ($n slam-ish strings)"
done
echo "--- pico's slam assets ---"
ls -l /system/etc/pvr/slam/ 2>/dev/null

echo
echo "########## RESTORE the pico stack and let it settle ##########"
start pvrservice 2>/dev/null
start airservice 2>/dev/null
sleep 3
am start -n com.pvr.vrshell/.MainActivity > /dev/null 2>&1
sleep 12
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"
echo "  vrshell    = $(pidof com.pvr.vrshell 2>/dev/null || echo DEAD)"

echo
echo "########## now poll the SAME ring buffer while pvrservice IS running ##########"
echo "  (if quality goes non-zero here, pvrservice is driving the tracker, not qvrservice)"
/vendor/bin/qvrservicetest64 -t 6 -d 20 -p 2000000 -l P 2>&1 | grep -E "pose_quality|position=|setting tracking|starting VR|ERROR|Failed" | head -30

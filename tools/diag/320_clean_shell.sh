#!/system/bin/sh
# Clean boot, nothing holding VR mode. Does the shell own VR mode and get to Unity?
echo "=== services ==="
for s in pn2_qvrd pvrservice airservice virtual_input; do
  printf "  %-16s %s\n" "$s" "$(getprop init.svc.$s)"
done
echo "  6dof.stopped   $(getprop pvr.service.6dof.stopped)"
echo
echo "=== launching shell ==="
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
echo "  pid $(pidof com.pvr.vrshell)"
echo
echo "=== did Unity start this time? ==="
logcat -d 2>/dev/null | grep -iE 'VrApi|UnityPlugin|Unity   :|EnterVrMode|TimeWarp:' | grep -viE 'Filename|mBitmap' | head -22
echo
echo "=== compositor ==="
echo "  BadPose  = $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  NoEyeBuf = $(logcat -d 2>/dev/null | grep -c 'No valid Eye')"
echo "  ts0      = $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0')"
echo
echo "=== did anything take VR mode? ==="
logcat -d 2>/dev/null | grep -iE 'VR Mode started|QVRConnection|has started VR mode' | tail -5

#!/system/bin/sh
# QVR allows only ONE VR-mode owner. My qvrservicetest "holder" was taking it
# exclusively, so the app could never acquire VR mode and stalled before Unity
# started - a 2D splash and no logs at all. Remove it and let the app own VR mode.
echo "=== killing the holder ==="
pkill -f qvrservicetest
sleep 2
pgrep -f qvrservicetest || echo "  gone"
echo
echo "=== clean launch of the shell ==="
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 22
echo "  pid: $(pidof com.pvr.vrshell)"
echo
echo "=== launch sequence ==="
logcat -d 2>/dev/null | grep -iE 'VrApi|UnityPlugin|Unity   :|EnterVrMode|TimeWarp' | grep -viE 'Filename|mBitmap' | head -25
echo
echo "=== compositor ==="
echo "  BadPose  = $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  NoEyeBuf = $(logcat -d 2>/dev/null | grep -c 'No valid Eye')"
echo "  ts0      = $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0')"

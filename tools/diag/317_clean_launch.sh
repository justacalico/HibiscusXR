#!/system/bin/sh
# The loading spinner is an Android FrameAnimation, not Unity - and Unity logs
# nothing. Capture a clean launch and find where it stalls.
am force-stop com.pvr.vrshell
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
V=$(pidof com.pvr.vrshell)
echo "=== pid $V ==="
echo
echo "=== launch sequence (VrApi / UnityPlugin / Unity / ConfigApi / VrService) ==="
logcat -d 2>/dev/null | grep -iE 'VrApi|UnityPlugin|Unity   :|ConfigApi|VrServiceApi|PvrClient|TimeWarp|EnterVrMode' | grep -viE 'Filename|mBitmap' | head -40
echo
echo "=== errors ==="
logcat -d -b crash 2>/dev/null | tail -8
logcat -d 2>/dev/null | grep -iE 'CANNOT LINK|UnsatisfiedLink|dlopen failed|Fatal|E AndroidRuntime' | tail -8

P=$(pidof com.pvr.vrshell)
echo "=== VRShell log, lines 55-160 ==="
logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo|FrameAnimation|ControllerClient|Undefined variable|avc:" | sed -n '55,160p'

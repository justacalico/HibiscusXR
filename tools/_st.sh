P=$(pidof com.pvr.vrshell)
echo "=== last 40 lines from vrshell ==="
logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo|FrameAnimation|Undefined variable|avc:" | tail -40
echo
echo "=== is it entering vr mode at all ==="
logcat -d | grep -icE "EnterVrMode"
echo "=== render threads present? ==="
ls /proc/$P/task | while read t; do cat /proc/$P/task/$t/comm 2>/dev/null; done | grep -iE "warp|unity|worker|render" | sort | uniq -c

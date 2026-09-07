logcat -c
am force-stop com.pvr.vrshell
sleep 2
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 25
echo "=== CVService crash header ==="
logcat -d | grep -E "signal |Abort message|Cause:|CVService|libPvr_UnitySDKCV" | grep -vE "^.*#[0-9]" | head -12
echo
echo "=== everything vrshell logged, tail ==="
P=$(pidof com.pvr.vrshell)
logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo" | tail -35
echo
echo "=== errors from the shell pid ==="
logcat -d | grep " $P " | grep -E " [EWF] " | grep -viE "Override displayinfo" | tail -20

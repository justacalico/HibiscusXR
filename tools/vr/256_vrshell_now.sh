#!/system/bin/sh
P=$(pidof com.pvr.vrshell)
echo "=== vrshell pid: ${P:-<not running>} ==="
[ -n "$P" ] && echo "threads: $(ls /proc/$P/task 2>/dev/null | wc -l)"
echo
echo "=== compositor: is it drawing or rejecting? ==="
logcat -d -t 500 2>/dev/null | grep -iE 'SelectRT|Bad Pose|Eye Buffer|WarpToScreen' | tail -6
echo
echo "=== did it enter vr mode / build the warp ==="
logcat -d 2>/dev/null | grep -iE 'EnterVrMode|Instantiate TimeWarp|InitRenderThread|hmdInfo\.widthPixels' | tail -6
echo
echo "=== what is on screen (focused window) ==="
dumpsys window 2>/dev/null | grep -iE 'mCurrentFocus|mFocusedApp' | head -4
echo
echo "=== unity activity ==="
logcat -d 2>/dev/null | grep -iE 'Unity   :' | tail -8

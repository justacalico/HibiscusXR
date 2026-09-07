#!/system/bin/sh
# The launcher is set as home but renders black. Confirm it is the pose rejection
# (compositor alive, frames submitted, every one refused) rather than the app
# failing to draw at all.
P=$(pidof com.pvr.launcher)
echo "=== launcher pid: ${P:-<not running>} ==="
[ -n "$P" ] && echo "threads: $(ls /proc/$P/task 2>/dev/null | wc -l)"
echo
echo "=== compositor state ==="
logcat -d -t 400 2>/dev/null | grep -iE 'SelectRT|Bad Pose|Eye Buffer|WarpToScreen|TimeWarp:' | tail -8
echo
echo "=== did it enter VR mode / init the render thread ==="
logcat -d 2>/dev/null | grep -iE 'EnterVrMode|InitRenderThread|hmdInfo\.|Instantiate TimeWarp' | tail -10
echo
echo "=== what pose does the tracker actually produce ==="
logcat -d -t 300 2>/dev/null | grep -iE 'getTrackingDataExt (rotation|position)' | tail -4
echo
echo "=== tracking mode + any 6dof errors ==="
logcat -d -t 400 2>/dev/null | grep -iE 'trackingMode|6dof|Get sensor|SensorState|force to 3dof' | tail -8
echo
echo "=== crashes ==="
logcat -d -b crash -t 100 2>/dev/null | grep -iE 'launcher|Fatal' | tail -5

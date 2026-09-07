echo "=== what activities does com.pvr.home expose? ==="
dumpsys package com.pvr.home | grep -A2 -iE "Activity Resolver|android.intent.action.MAIN" | head -20
echo
echo "=== launch it ==="
logcat -c
am start -n com.pvr.home/.RecActivity 2>&1 | head -5
sleep 20
echo "  pid $(pidof com.pvr.home)"
echo "=== focus ==="
dumpsys window | grep mCurrentFocus
echo "=== errors from home ==="
P=$(pidof com.pvr.home); logcat -d | grep " $P " | grep -E " [EWF] " | grep -viE "Override displayinfo|Undefined variable" | tail -18
echo "=== did it enter VR mode / render? ==="
logcat -d | grep -iE "EnterVrMode|TimeWarp|Unity|Boundary|Seethrough" | tail -12

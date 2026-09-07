echo "--- what the app exposes ---"
dumpsys package com.pvr.seethrough.setting | grep -B1 -A3 "android.intent.action.MAIN" | head -20
echo
echo "--- match stock: seethrough enabled ---"
setprop persist.pvrcon.seethrough.enable 1
echo "  persist.pvrcon.seethrough.enable = $(getprop persist.pvrcon.seethrough.enable)"
echo
echo "--- airservice is what feeds the camera passthrough; make sure it is up ---"
start airservice 2>/dev/null
sleep 3
echo "  airservice pid $(pidof airservice)"
echo
logcat -c
echo "--- launch ---"
am start -n com.pvr.seethrough.setting/com.unity3d.player.UnityPlayerNativeActivityPico 2>&1 | head -3
sleep 20
P=$(pidof com.pvr.seethrough.setting)
echo "  pid $P  threads=$(ls /proc/$P/task 2>/dev/null | wc -l)"
echo "--- focus ---"
dumpsys window | grep mCurrentFocus
echo "--- crashes ---"
logcat -d | grep -E "E CRASH|signal 11|FATAL" | head -6
echo "  (empty = no crash)"
echo "--- camera / passthrough ---"
logcat -d | grep -iE "airservice|aircamera|seethrough|passthrough|QVRServiceCamDeviceHAL3|camera" | grep -viE "LockBuffer" | tail -14
echo "--- app log ---"
logcat -d | grep " $P " | grep -viE "chatty|Undefined variable|avc:" | tail -20

#!/system/bin/sh
echo "=== displays known to the framework ==="
dumpsys display 2>/dev/null | grep -iE 'mDisplayId|uniqueId|DisplayDeviceInfo|state=|flags=|density|type=' | head -40
echo
echo "=== wm size/density ==="
wm size 2>&1
wm density 2>&1
echo
echo "=== input devices ==="
cat /proc/bus/input/devices 2>/dev/null | grep -E '^N:|^H:|^B: KEY' | head -40
echo
echo "=== framework input devices ==="
dumpsys input 2>/dev/null | grep -iE 'Device -?[0-9]+:|Sources:|KeyboardType|ExternalStylus|DisplayViewport' | head -40
echo
echo "=== viewports (scrcpy injects into the default one) ==="
dumpsys input 2>/dev/null | grep -iA3 'Viewports' | head -30
echo
echo "=== current top activity ==="
dumpsys activity activities 2>/dev/null | grep -iE 'mResumedActivity|topResumedActivity' | head -5
echo
echo "=== does plain injection work? tap + keyevent ==="
input tap 500 500 2>&1; echo "tap rc=$?"
input keyevent 82 2>&1; echo "menu rc=$?"
echo
echo "=== stay-awake / lock state ==="
dumpsys power 2>/dev/null | grep -iE 'mWakefulness|mHoldingDisplay|Display Power' | head -6
dumpsys window 2>/dev/null | grep -iE 'mDreamingLockscreen|mShowingLockscreen|mAwake' | head -5
echo DONE

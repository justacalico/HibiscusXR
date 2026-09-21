#!/system/bin/sh
# Are my stubbed CpuConsumer helpers actually on the passthrough path? They
# warn_once to logcat. If that warning never fires, the white image has another
# cause and reimplementing them would be wasted work.
echo "=== has the shim warned? ==="
logcat -d | grep -i shim_air | tail -6
echo "  (empty = the stubs were never called)"

echo
echo "=== is the shim even loaded into airservice? ==="
A=$(pidof airservice)
echo "  airservice pid $A"
grep -oE '/[^ ]*libshim_air[^ ]*' /proc/$A/maps 2>/dev/null | sort -u
grep -oE '/[^ ]*libaircamera[^ ]*' /proc/$A/maps 2>/dev/null | sort -u

echo
echo "=== is airservice getting camera frames at all? ==="
logcat -d | grep -iE 'aircamera|AIRService.*[Cc]amera|processCameraStateChange|onFrame|frameAvailable' | grep -v LockBuffer | tail -14

echo
echo "=== camera devices open ==="
logcat -d | grep -iE 'QVRServiceCamDeviceHAL3.*(Start|Stop|open|error)' | tail -8

echo
echo "=== what does the app think it is showing ==="
S=$(pidof com.pvr.seethrough.setting)
logcat -d | grep " $S " | grep -viE 'chatty|Undefined variable|avc:' | tail -18

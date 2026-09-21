#!/system/bin/sh
# The "Movement Tracking Lost" dialog offers:
#   [Back]    disable Movement Tracking  -> the 3DoF path we want
#   [Confirm] reset Movement Tracking
#
# Android's KEYCODE_BACK did nothing (mCurrentFocus=null - the VR app takes input
# through Pico's stack, not Android focus). Inject at the evdev level instead, on
# the pvr-virtual-input nodes the virtual_input daemon creates. KEY_BACK = 158.
KEY=158
for N in 5 6 7 8 9; do
  D=/dev/input/event$N
  [ -c "$D" ] || continue
  NAME=$(getevent -pl "$D" 2>/dev/null | grep -m1 'name:' | sed 's/.*name: *//')
  case "$NAME" in
    *pvr-virtual-input*)
      echo "sending KEY_BACK on $D ($NAME)"
      sendevent $D 1 $KEY 1
      sendevent $D 0 0 0
      sendevent $D 1 $KEY 0
      sendevent $D 0 0 0
      ;;
  esac
done
sleep 4
echo
echo "=== compositor state after ==="
logcat -d -t 120 2>/dev/null | grep -iE 'SelectRT|trackingstate|Bad Pose|Nothing to draw|tracking' | tail -10

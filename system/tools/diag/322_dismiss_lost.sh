#!/system/bin/sh
# Unity IS running (UnityMain/WarpThread/Job.Workers present) and the compositor is
# alive. The "Movement Tracking Lost" dialog is the app's own error state, driven by
# trackingstate=0 via BoundarySystem::CalculateDialogState -> kLostDialog.
#
# Keycode 158 on the pvr-virtual-input nodes is the dialog's "disable Movement
# Tracking" action and previously flipped the compositor from rejecting poses to
# accepting them. Send it now that Unity is actually up.
echo "=== before ==="
logcat -c; sleep 6
echo "  BadPose  = $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  kLost    = $(logcat -d 2>/dev/null | grep -c 'kLostDialog')"
echo "  ts0      = $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0')"

echo
echo "=== sending 158 (disable movement tracking) ==="
for N in 5 6 7 8 9; do
  D=/dev/input/event$N
  [ -c "$D" ] || continue
  sendevent $D 1 158 1; sendevent $D 0 0 0
  sendevent $D 1 158 0; sendevent $D 0 0 0
done

logcat -c
sleep 12
echo "=== after ==="
echo "  BadPose  = $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  kLost    = $(logcat -d 2>/dev/null | grep -c 'kLostDialog')"
echo "  ts0      = $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0')"
echo "  NoEyeBuf = $(logcat -d 2>/dev/null | grep -c 'No valid Eye')"
echo
echo "=== unity scene activity ==="
logcat -d 2>/dev/null | grep -iE 'Unity   :' | grep -viE 'Filename' | tail -12

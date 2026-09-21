#!/system/bin/sh
# Keycode 158 was the only one that stopped the Bad Pose rejections. Confirm that
# over a longer window and see whether the compositor actually draws.
echo "=== baseline (10s, no key) ==="
logcat -c; sleep 10
echo "  Bad Pose: $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  drew:     $(logcat -d 2>/dev/null | grep -c 'WarpToScreen')"
echo
echo "=== sending 158 ==="
for N in 5 6 7 8 9; do
  D=/dev/input/event$N
  [ -c "$D" ] || continue
  sendevent $D 1 158 1; sendevent $D 0 0 0
  sendevent $D 1 158 0; sendevent $D 0 0 0
done
logcat -c
sleep 12
echo "  Bad Pose: $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  Nothing to draw: $(logcat -d 2>/dev/null | grep -c 'Nothing to draw')"
echo "  trackingstate 0: $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0,0x0')"
echo
echo "=== any sign of actual rendering ==="
logcat -d 2>/dev/null | grep -iE 'SelectRT|WarpToScreen|Eye Buffer|thisEyeBufferNum' | tail -8

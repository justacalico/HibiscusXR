#!/system/bin/sh
# The dialog offers Confirm = reset tracking, Back = DISABLE tracking (the 3DoF
# path we want). Sending KEY_BACK(158) earlier reset the pose to identity, which is
# the Confirm behaviour - so the mapping is not what I assumed.
#
# Sweep the keys the pvr-virtual-input devices expose and watch whether the
# compositor stops rejecting frames after each one.
DEVS=""
for N in 5 6 7 8 9; do
  D=/dev/input/event$N
  [ -c "$D" ] && DEVS="$DEVS $D"
done

send() {  # send <code>
  for D in $DEVS; do
    sendevent $D 1 $1 1; sendevent $D 0 0 0
    sendevent $D 1 $1 0; sendevent $D 0 0 0
  done
}

rejects() {
  logcat -d -t 60 2>/dev/null | grep -c 'Bad Pose'
}

for K in 158 172 139 28 353 keyname; do
  [ "$K" = "keyname" ] && continue
  echo "=== keycode $K ==="
  logcat -c
  send $K
  sleep 4
  R=$(rejects)
  echo "  Bad Pose lines in last 60: $R"
  logcat -d -t 40 2>/dev/null | grep -iE 'Nothing to draw|WarpToScreen|trackingstate|VRBoundary|tracking' | tail -3
  echo
done

#!/system/bin/sh
# Who keeps QVR VR mode open on this device? That is the piece we are missing:
# with VR mode held, our cameras opened and the DSP tracker produced real
# positional data.
echo "=== processes with a qvrservice socket open ==="
for p in $(ls /proc 2>/dev/null | grep -E '^[0-9]+$'); do
  if ls -l /proc/$p/fd 2>/dev/null | grep -q 'qvrservice'; then
    echo "  pid $p = $(cat /proc/$p/comm 2>/dev/null)  [$(cat /proc/$p/cmdline 2>/dev/null | tr '\0' ' ')]"
  fi
done
echo
echo "=== processes linking the qvr client library ==="
for p in $(ls /proc 2>/dev/null | grep -E '^[0-9]+$'); do
  if grep -q 'libqvrservice_client' /proc/$p/maps 2>/dev/null; then
    echo "  pid $p = $(cat /proc/$p/comm 2>/dev/null)"
  fi
done
echo
echo "=== VR mode state ==="
getprop | grep -iE 'vrmode|vr_mode|6dof.stopped|vrservice.state'
echo
echo "=== QVR connection log ==="
logcat -d 2>/dev/null | grep -iE 'QVRConnection|VR Mode started|VR Mode stopp|has started VR mode' | tail -8

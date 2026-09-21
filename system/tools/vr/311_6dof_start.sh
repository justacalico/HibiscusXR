#!/system/bin/sh
# Only pvr.service.6dof.stopped differs from stock (ours true, stock false).
# pvrservice logged "force to 3dof by wake up". Compare how 6DoF is started.
echo "=== 6dof-related pvrservice lines ==="
logcat -d 2>/dev/null | grep -iE 'PvrService|PvrPlatform|6dof|Paul|SensorConstruct|OrientationTracker' | grep -iE '6dof|3dof|tracker|start|stop|mode' | tail -25
echo
echo "=== the props ==="
getprop pvr.service.6dof.stopped
getprop pxr.service.6dof.restarted
getprop persist.pvrservice.trackingmode
echo
echo "=== does pvrservice link the qvr client? ==="
P=$(pidof pvrservice)
grep -oE '/[^ ]*qvr[^ ]*\.so' /proc/$P/maps 2>/dev/null | sort -u
echo "  (blank = it does not)"
echo
echo "=== which process holds the qvrservice socket ==="
for p in $(ls /proc | grep -E '^[0-9]+$'); do
  if ls -l /proc/$p/fd 2>/dev/null | grep -q 'qvrservice'; then
    echo "  pid $p = $(cat /proc/$p/comm 2>/dev/null)"
  fi
done

#!/system/bin/sh
# On stock, libqvrservice_client is linked by pvrservice AND cvcontroller:RemoteService.
# We disabled CVService earlier because it crash-looped in a wifi broadcast. Now
# that the DSP stack works, re-enable it: it is one of the two things that holds
# QVR VR mode, and with VR mode held our cameras opened and the tracker produced
# real positional data.
echo "=== enabling CVService ==="
pm enable com.picovr.picovrlib.cvcontroller
setprop pvr.display.vrmode.enable 1
echo
echo "=== restart the VR stack in the right order ==="
stop pvrservice; sleep 2; start pvrservice; sleep 4
am force-stop com.pvr.vrshell
am force-stop com.pvr.seethrough.setting
sleep 2
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 20
echo
echo "=== does anything link the qvr client now? ==="
for p in $(ls /proc 2>/dev/null | grep -E '^[0-9]+$'); do
  if grep -q 'libqvrservice_client' /proc/$p/maps 2>/dev/null; then
    echo "  pid $p = $(cat /proc/$p/comm 2>/dev/null)"
  fi
done
echo
echo "=== VR mode / 6dof state ==="
getprop pvr.service.6dof.stopped
logcat -d 2>/dev/null | grep -iE 'VR Mode started|QVRConnection|CamDeviceHAL3|force to 3dof' | tail -8
echo
echo "=== compositor ==="
logcat -d -t 200 2>/dev/null | grep -iE 'SelectRT|Bad Pose|trackingstate' | tail -5

#!/system/bin/sh
# Capture what PVRSERVICE logs while VRShell starts, on either device.
#
# Theory: psmvr_UpdateLensAndDisplayInfoFromVRService asks pvrservice for lens
# and display info. If the service answers, done. If it does not, the SDK falls
# back to a JNI call into Java - and at that point PvrClient has not been created
# yet (stock logs "PvrClient created" AFTER UpdateLensInfo), so the object is
# null and the call faults. That would mean our crash is a fallback stock never
# takes, and the real defect is pvrservice not answering.
exec 2>&1
V=$(pidof pvrservice)
echo "pvrservice pid: $V"
logcat -c
am force-stop com.pvr.vrshell
sleep 1
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 6
echo
echo "##### everything pvrservice logged during the launch #####"
logcat -d -v threadtime | grep -E "^[0-9-]+ [0-9:.]+ +$V " | sed 's/^[0-9-]* [0-9:.]* *//' \
  | grep -viE 'getTrackingDataExt|threadLoop continue|setShareMemoryData|identical|stationary' | head -40
echo
echo "##### lens / display / parameter traffic from anyone #####"
logcat -d | grep -iE 'lens|getParameter|UpdateLens|UpdateDisplay|displayinfo|frustum|fov' \
  | sed 's/^[0-9-]* [0-9:.]* *//' | head -25
echo DONE

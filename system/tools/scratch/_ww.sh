logcat -c
am force-stop com.pvr.vrshell
sleep 2
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 22
P=$(pidof com.pvr.vrshell)
echo "=== VRShell startup, unfiltered, first 60 ==="
logcat -d | grep " $P " | grep -viE "chatty|Override displayinfo|FrameAnimation|ControllerClient" | head -60
echo
echo "=== does anything reference pvr_manager / ConfigurationService ==="
logcat -d | grep -iE "pvr_manager|IPvrManagerService|ConfigurationService|IConfigService" | tail -10
echo
echo "=== is com.pvr.configuration alive and did it try to register ==="
pidof com.pvr.configuration
logcat -d | grep -iE "com.pvr.configuration" | tail -8

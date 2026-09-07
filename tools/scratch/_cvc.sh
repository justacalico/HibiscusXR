echo "=== fix the label I left unset when I created the arm dir ==="
mount -o rw,remount /system
ls -Zd /system/priv-app/CVService/lib /system/priv-app/CVService/lib/arm
chcon -R u:object_r:system_file:s0 /system/priv-app/CVService 2>/dev/null
ls -Zd /system/priv-app/CVService/lib/arm
sync
echo
echo "=== full crash, with the lines above the SDK frames ==="
logcat -c
am force-stop com.picovr.picovrlib 2>/dev/null
am force-stop com.pvr.cvservice 2>/dev/null
sleep 1
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 20
logcat -d | grep -A45 "backtrace:" | head -55
echo
echo "=== what CVService logged before dying ==="
logcat -d | grep -iE "RemoteService|CVService|psmvr|InitServiceClient" | grep -v "avc:" | tail -25

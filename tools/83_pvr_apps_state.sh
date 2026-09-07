#!/system/bin/sh
echo "=== which pico processes are alive ==="
ps -A -o PID,UID,NAME 2>/dev/null | grep -iE 'pvr|pico' | grep -v pvrservice
echo
echo "pvrservice: $(pidof pvrservice)"
echo

echo "=== did they get system uid? (uid 1000 = android.uid.system granted) ==="
for p in com.pvr.vrshell com.pvr.vrdisplay com.picovr.picovrlib.cvcontroller com.pvr.adapter; do
  u=$(dumpsys package $p 2>/dev/null | grep -m1 'userId=')
  echo "$p  $u"
done
echo

echo "=== launchable entry points ==="
for p in com.pvr.vrshell com.picovr.vrusercenter com.pvr.vrdisplay; do
  echo "--- $p ---"
  dumpsys package $p 2>/dev/null | grep -A3 'android.intent.action.MAIN' | head -8
done
echo

echo "=== any pico app crashes so far ==="
logcat -d -b crash 2>/dev/null | grep -iE 'pvr|pico' | tail -10
echo
echo "=== pico app logs ==="
logcat -d 2>/dev/null | grep -iE 'PvrDisplay|VRShell|CVService|pvr_adapter|PxrNotif|pvrverify' | tail -20
echo DONE

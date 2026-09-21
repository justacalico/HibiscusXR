#!/system/bin/sh
# monkey found no LAUNCHER activity for com.pvr.home. Find the real entry points
# for the /oem-derived apps, and see what the crashing service is.
exec 2>&1
for p in com.pvr.home com.pvr.launcher com.picovr.store com.picovr.provision com.pvr.tobservice; do
  echo "##### $p #####"
  echo "  -- activities with MAIN --"
  dumpsys package $p 2>/dev/null | grep -B2 -A2 'android.intent.action.MAIN' | grep -E '^\s+[0-9a-f]+ ' | head -5
  echo "  -- categories --"
  dumpsys package $p 2>/dev/null | grep -i 'Category:' | sort -u | head -5
  echo
done
echo "##### which pico process is crash-looping #####"
logcat -d | grep -E 'AndroidRuntime: Process:' | sort -u | tail -8
echo
echo "##### the actual exception #####"
logcat -d | grep -E 'AndroidRuntime' | grep -vE '^\s*at |^.*E AndroidRuntime: \s' | grep -iE 'Exception|Error|Caused' | tail -8
echo DONE

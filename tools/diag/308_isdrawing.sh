#!/system/bin/sh
echo "=== what is running ==="
for p in com.pvr.vrshell com.pvr.seethrough.setting com.pvr.launcher; do
  printf "  %-32s %s\n" "$p" "$(pidof $p)"
done
echo
echo "=== 8 second compositor sample ==="
logcat -c
sleep 8
echo "  Bad Pose        : $(logcat -d 2>/dev/null | grep -c 'Bad Pose')"
echo "  No valid Eye Buf: $(logcat -d 2>/dev/null | grep -c 'No valid Eye Buffers')"
echo "  Nothing to draw : $(logcat -d 2>/dev/null | grep -c 'Nothing to draw')"
echo "  trackingstate 0 : $(logcat -d 2>/dev/null | grep -c 'trackingstate = 0x0,0x0')"
echo "  WarpSwap/present: $(logcat -d 2>/dev/null | grep -cE 'WarpSwap|TimeWarpEvent')"
echo
echo "=== last compositor lines ==="
logcat -d 2>/dev/null | grep -iE 'SelectRT|TimeWarp:' | tail -6
echo
echo "=== last Unity lines ==="
logcat -d 2>/dev/null | grep -iE 'Unity   :' | grep -viE 'Filename|^\s*$' | tail -8

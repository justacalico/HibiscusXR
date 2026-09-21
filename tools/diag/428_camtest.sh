#!/system/bin/sh
# Frame delivery is dark-valid: a dark frame is still a frame. This proves the
# QVRCamera client path works with nothing of pico's running, which is the other
# half of what the monado driver needs.
set -u
echo "########## release VR mode ##########"
stop pvrservice 2>/dev/null; stop airservice 2>/dev/null
sleep 2
for p in pvrservice airservice; do
  pid=$(pidof $p 2>/dev/null); [ -n "$pid" ] && kill -9 $pid 2>/dev/null
done
sleep 2
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  qvrservice = $(pidof qvrservice 2>/dev/null || echo DEAD)"

echo
echo "########## camera client, standalone ##########"
logcat -c 2>/dev/null
timeout 30 /vendor/bin/qvrcameratest64 -n monado_probe 2>&1 | head -40
echo "  --- exit: $? ---"

echo
echo "########## service side ##########"
logcat -d 2>/dev/null | grep -iE "QVRCamera|CamDevice|frame|ion buffer|exposure|stream" | tail -20

echo
echo "########## restore ##########"
start pvrservice 2>/dev/null; start airservice 2>/dev/null
sleep 3
echo "  pvrservice = $(pidof pvrservice 2>/dev/null || echo DEAD)"
echo "  airservice = $(pidof airservice 2>/dev/null || echo DEAD)"

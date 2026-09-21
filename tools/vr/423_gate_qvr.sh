#!/system/bin/sh
# GATE 1: does Qualcomm's qvrservice deliver 6DoF with Pico's stack completely dead?
# If yes, the whole Monado plan is just implementation from here.
set -u

echo "########## BEFORE ##########"
echo "  init services:"
getprop | grep "init.svc" | grep -iE "pvr|qvr|air|fan" | sed 's/^/    /'
echo "  processes:"
for p in pvrservice airservice qvrservice fancontrol adsprpcd cdsprpcd; do
  echo "    $p = $(pidof $p 2>/dev/null || echo -)"
done
echo "  pico apps:"
pgrep -l -f "com.pvr" 2>/dev/null | sed 's/^/    /' || ps -A -o PID,NAME 2>/dev/null | grep com.pvr | sed 's/^/    /'

echo
echo "########## STOPPING PICO LAYER ONLY ##########"
for a in com.pvr.vrshell com.pvr.launcher com.pvr.vrdisplay com.pvr.verify \
         com.pvr.configuration com.pvr.pxrnotification com.picovr.picovrlib.cvcontroller; do
  am force-stop "$a" 2>/dev/null
done
# init-managed daemons
for s in pn2_pvrservice pvrservice pn2_airservice airservice; do
  stop "$s" 2>/dev/null
done
sleep 1
# anything left
for p in pvrservice airservice; do
  pid=$(pidof $p 2>/dev/null)
  [ -n "$pid" ] && kill -9 $pid 2>/dev/null && echo "    killed $p ($pid)"
done
sleep 2

echo "  after stop:"
for p in pvrservice airservice qvrservice fancontrol; do
  echo "    $p = $(pidof $p 2>/dev/null || echo DEAD)"
done

echo
echo "########## GATE TEST: 6DoF, 25 s, poll 250 ms ##########"
echo "  (qvrservice must StartVRMode with no Pico client present)"
logcat -c 2>/dev/null
/vendor/bin/qvrservicetest64 -t 6 -d 25 -p 250000 -l P 2>&1 | tail -60

echo
echo "########## qvrservice side of the story ##########"
logcat -d 2>/dev/null | grep -iE "qvr|vrmode|tracking|pose" | tail -25

echo
echo "########## STILL ALIVE? ##########"
for p in qvrservice fancontrol adsprpcd cdsprpcd; do
  echo "    $p = $(pidof $p 2>/dev/null || echo DEAD)"
done
echo "  fan rpm = $(cat /sys/class/hwmon/hwmon1/fan1_input 2>/dev/null)"

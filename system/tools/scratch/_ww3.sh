logcat -c
am force-stop com.pvr.vrshell
sleep 2
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 22
P=$(pidof com.pvr.vrshell)
logcat -d > /data/local/tmp/full.log
grep " $P " /data/local/tmp/full.log | grep -viE "chatty|Override displayinfo|FrameAnimation|ControllerClient|Undefined variable|avc:" > /data/local/tmp/vs.log
echo "total lines: $(wc -l < /data/local/tmp/vs.log)"
echo "=== 50-150 ==="
sed -n '50,150p' /data/local/tmp/vs.log

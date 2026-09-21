echo "--- pvrservice health before ---"
P=$(pidof pvrservice)
echo "  pid $P  malloc-contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)"
logcat -c
echo "--- re-enable ---"
pm enable com.picovr.picovrlib.cvcontroller 2>&1 | tail -1
sleep 3
am startservice com.picovr.picovrlib.cvcontroller/.cvcontrollerlib.CVControllerService >/dev/null 2>&1
echo "--- watch 40s: does it settle or crash-loop? ---"
i=0
while [ $i -lt 4 ]; do
  sleep 10
  C=$(logcat -d | grep -c "exited due to signal 11")
  R=$(logcat -d | grep -c "registerClient")
  echo "  t+$(( (i+1)*10 ))s  segv=$C  registerClient=$R  cvPid=$(pidof com.picovr.picovrlib.cvcontroller:RemoteService)"
  i=$((i+1))
done
echo "--- pvrservice health after ---"
P=$(pidof pvrservice)
echo "  pid $P  malloc-contended=$(debuggerd -b $P 2>&1 | grep -c je_malloc_mutex)"
echo "  zombie am children=$(ps -A -o PID,PPID,STAT 2>/dev/null | awk -v p=$P \x27$2==p && $3 ~ /Z/\x27 | wc -l)"
echo "--- is tracking still good? ---"
echo "  BadPose=$(logcat -d | grep -c \"Bad Pose\")  kLost=$(logcat -d | grep -c kLostDialog)"
logcat -d | grep -iE "getTrackingDataExt position" | tail -2
echo "--- controller seen? ---"
logcat -d | grep -iE "ControllerClient|controller_type|bind service" | tail -6

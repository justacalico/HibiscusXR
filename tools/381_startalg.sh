#!/system/bin/sh
echo "=== full crash in StartAlgorithm ==="
logcat -d | grep -B6 -A16 'AIRClient::StartAlgorithm' | head -40

echo
echo "=== which process died ==="
logcat -d | grep -E '>>> .* <<<' | tail -3
logcat -d | grep -E 'signal .*SIGSEGV|Cause:|fault addr' | tail -4

echo
echo "=== did the algorithm actually load its symbols ==="
logcat -d | grep -iE 'SafetyAreaRecovery|loadSymbols|startAlgorithm|AlgorithmManager' | tail -12

echo
echo "=== current state ==="
echo "  seethrough pid [$(pidof com.pvr.seethrough.setting)]"
echo "  airservice pid [$(pidof airservice)]"
echo "  vrshell    pid [$(pidof com.pvr.vrshell)]"
dumpsys window 2>/dev/null | grep -m2 mCurrentFocus

echo
echo "=== is the camera streaming to the app ==="
logcat -d | grep -iE 'aircamera|processCameraStateChange|CamDeviceHAL3' | grep -v LockBuffer | tail -8

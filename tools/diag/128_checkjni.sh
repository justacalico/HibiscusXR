#!/system/bin/sh
# The crash is inside art_quick_generic_jni_trampoline with sp=0 and only one
# usable frame, which tells us almost nothing. CheckJNI makes ART validate every
# JNI call and abort with a descriptive message naming the method and the
# problem, instead of walking off a bad frame.
exec 2>&1
setprop debug.checkjni 1
am force-stop com.pvr.vrshell
logcat -c
am start -n com.pvr.vrshell/.MainActivity >/dev/null 2>&1
sleep 8
echo "vrshell pid: $(pidof com.pvr.vrshell)"
echo
echo "=== CheckJNI / JNI complaints ==="
logcat -d | grep -iE 'JNI|checkjni|native method|signature|GetMethodID|CallVoid|RegisterNatives' | tail -25
echo
echo "=== tail around the failure ==="
logcat -d | grep -iE 'ConfigApi|psmvr|UnityNative|VrApi|SIGSEGV|Fatal|abort' | tail -15
echo DONE

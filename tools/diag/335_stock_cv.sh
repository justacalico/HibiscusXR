#!/system/bin/sh
# Does the stock headset's CVService crash the same way? If it does, this crash
# is noise and the Loading stall has another cause.
echo "=== is the controller service alive? ==="
ps -A 2>/dev/null | grep -iE "cvcontroller|RemoteService" | head -5
echo
echo "=== any SIGSEGV / pvrserviceclient crashes in the buffer? ==="
logcat -d 2>/dev/null | grep -cE "libpvrserviceclient|getPvrService"
logcat -d 2>/dev/null | grep -E "signal 11|libpvrserviceclient" | head -8
echo
echo "=== ControllerClient connect churn (ours loops forever) ==="
logcat -d 2>/dev/null | grep -c "scv ServiceConnection"
echo
echo "=== who is the launcher/home here ==="
dumpsys window 2>/dev/null | grep -E "mCurrentFocus"

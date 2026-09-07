echo "=== what has focus right now ==="
dumpsys window | grep -E "mCurrentFocus|mFocusedApp" | head -3
echo
echo "=== which pvr apps are alive ==="
ps -A | grep -iE "pvr.home|pvr.vrshell|cvcontroller" | awk '{print "  "$1" "$NF}'
echo
echo "=== controller service state (I disabled it) ==="
pm list packages -d | grep -i cvcontroller || echo "  not disabled"
echo
echo "=== tracking-lost dialog in the current buffer? ==="
echo "  kLostDialog=$(logcat -d | grep -c kLostDialog)"

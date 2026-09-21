#!/system/bin/sh
# The "Movement Tracking Lost" dialog can disable tracking via the headset Back
# button. Find where that choice is stored so it can be set directly - the Unity
# app does not take Android key events (mCurrentFocus is null).
echo "=== vrshell app storage ==="
ls -l /data/data/com.pvr.vrshell/shared_prefs/ 2>/dev/null
for f in /data/data/com.pvr.vrshell/shared_prefs/*.xml; do
  [ -f "$f" ] && echo "--- $f ---" && cat "$f"
done 2>/dev/null
echo
echo "=== unity PlayerPrefs live here too ==="
ls -l /data/data/com.pvr.vrshell/files 2>/dev/null | head
echo
echo "=== stock, for comparison ==="
echo "(run on stock separately)"
echo
echo "=== any tracking-related settings/props the shell might read ==="
getprop 2>/dev/null | grep -iE '6dof|3dof|tracking|recline|movement'
echo "--- settings ---"
for ns in system secure global; do settings list $ns 2>/dev/null | grep -iE '6dof|3dof|tracking|recline'; done

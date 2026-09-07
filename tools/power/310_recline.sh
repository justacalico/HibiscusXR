#!/system/bin/sh
# Two routes now:
#   A) hold QVR VR mode open -> cameras + SLAM work (proven: HAL3 opened, airservice
#      connected) - needs a persistent holder
#   B) force recline / 3DoF so the shell stops waiting for SLAM
# Find B's switch. Pico may not call it "recline" internally.
echo "=== qvrservicetest options (can it hold VR mode indefinitely?) ==="
/vendor/bin/qvrservicetest64 -h 2>&1 | head -20
echo
echo "=== strings in the settings/shell apks for recline-ish terms ==="
for f in /system/priv-app/PVRLauncher/PVRLauncher.apk /system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so; do
  [ -f "$f" ] || continue
  echo "--- $f ---"
  strings -a "$f" 2>/dev/null | grep -iE 'recline|lie_?down|lying|sit_?mode|3dof|sixdof|six_dof' | sort -u | head -12
done
echo
echo "=== pvr props that look like a tracking-mode switch ==="
getprop 2>/dev/null | grep -iE 'recline|lie|sit|3dof|6dof|tracking'
echo
echo "=== settings ==="
for ns in system secure global; do settings list $ns 2>/dev/null | grep -iE 'recline|lie|sit|3dof|6dof|track'; done

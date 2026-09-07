#!/system/bin/sh
# Run on EITHER device. Hash the Pico libs so we can tell whether the port is
# running the same binaries the stock unit does. Our copies came from the OTA
# 4.1.3 image; the stock unit is on B346. If those differ, every conclusion
# drawn from our extracted copies is suspect.
for f in /system/lib64/libpxr_6dof_optimization.so \
         /system/lib64/lib6DofReset.so \
         /system/lib64/libpvrserviceclient.so \
         /system/lib64/libpvrmodule_platform.so \
         /system/lib64/libcompositor.pxr.so \
         /system/lib64/libruntime.pxr.so \
         /system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so; do
  if [ -f "$f" ]; then
    echo "$(md5sum "$f" 2>/dev/null | cut -d' ' -f1)  $(stat -c%s "$f")  $f"
  else
    echo "MISSING                           -  $f"
  fi
done

#!/bin/bash
# Extract the Pico VR runtime from the stock system.img.
#
# /vendor still has the QVR/PVR *client* libs, but everything that actually runs
# - the daemons, the compositor, the 6DoF solver, the config - lived on the stock
# /system that the GSI replaced. Pull it back out with debugfs (image is never
# mounted, never written).
#
# Skipping libPvr_UnitySDK*.so for now: ~60MB of app-side SDK runtime that
# pvrservice itself does not load. Add later if an app needs it.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
OUT=/mnt/f/PN2Lineage/pvr_stack
LOG=/mnt/f/PN2Lineage/notes/66_extract.txt
exec >"$LOG" 2>&1

rm -rf "$OUT"
mkdir -p "$OUT"/{bin,lib,lib64,framework,etc/init}

pull() {  # pull <src-in-image> <dst>
  debugfs -R "dump $1 $2" "$IMG" 2>/dev/null
  if [ -s "$2" ]; then
    printf '  %-46s %8d bytes\n' "$1" "$(stat -c%s "$2")"
  else
    printf '  %-46s MISSING\n' "$1"
    rm -f "$2"
  fi
}

echo "=== daemons -> bin ==="
for f in pvrservice qvrservice pvr_compute vr; do pull "/bin/$f" "$OUT/bin/$f"; done

echo
echo "=== 64-bit libs ==="
for f in libcompositor.pxr.so libruntime.pxr.so libloader.pxr.so libplugin.pxr.so \
         libpvrservice.so libpvrserviceclient.so libpvrmodule_orientationtracker.so \
         libpvrmodule_platform.so libpxr_6dof_optimization.so \
         libCVControllerClient.pxr.so libconfigurationclient.pxr.so \
         libpxrnotification.pxr.so libpxrserviceclient.so \
         libqvrcamera_client_system.so; do
  pull "/lib64/$f" "$OUT/lib64/$f"
done

echo
echo "=== 32-bit libs (qvrservice is 32-bit only) ==="
for f in libcompositor.pxr.so libruntime.pxr.so libloader.pxr.so libplugin.pxr.so \
         libpvrservice.so libpvrserviceclient.so libpvrmodule_orientationtracker.so \
         libpvrmodule_platform.so libpxr_6dof_optimization.so \
         libCVControllerClient.pxr.so libconfigurationclient.pxr.so \
         libpxrnotification.pxr.so libpxrserviceclient.so \
         libqvrservice.so libqvrcamera_client_system.so \
         libqvr_eyetracking_plugin.so libqvr_mapper_stub.so \
         libqvr_cdsp_driver_stub.so libqvr_cam_cdsp_driver_stub.so; do
  pull "/lib/$f" "$OUT/lib/$f"
done

echo
echo "=== framework ==="
for f in pxr_sdk_api.jar vr.jar; do pull "/framework/$f" "$OUT/framework/$f"; done

echo
echo "=== init script ==="
pull "/etc/init/pvrservice.rc" "$OUT/etc/init/pvrservice.rc"

echo
echo "=== /etc/pvr (recursive) ==="
debugfs -R "rdump /etc/pvr $OUT/etc" "$IMG" 2>/dev/null
find "$OUT/etc/pvr" -type f 2>/dev/null | while read -r f; do
  printf '  %-56s %8d bytes\n' "${f#$OUT/}" "$(stat -c%s "$f")"
done

echo
echo "=== what pvrservice.rc actually says ==="
cat "$OUT/etc/init/pvrservice.rc" 2>/dev/null

echo
echo "=== totals ==="
du -sh "$OUT"
find "$OUT" -type f | wc -l
echo DONE

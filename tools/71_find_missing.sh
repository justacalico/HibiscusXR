#!/bin/bash
# Two jobs:
#  1. find the pvr modules I failed to extract the first time
#  2. find out how many of the PVR blobs reference framework symbols that
#     Android 10 removed - getBuiltInDisplay is one, there may be more.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
OUT=/mnt/f/PN2Lineage/pvr_stack
LOG=/mnt/f/PN2Lineage/notes/71_missing.txt
exec >"$LOG" 2>&1

echo "=== every libpvrmodule_* / calibrate / shim-ish lib in the stock image ==="
for d in /lib64 /lib; do
  echo "--- $d ---"
  debugfs -R "ls -l $d" "$IMG" 2>/dev/null \
    | grep -iE 'pvrmodule|HeadImu|Calibrate|externalhmd|controller|pvrclient|svr|wvr' \
    | awk '{print $NF, $(NF-4)}'
done
echo

echo "=== pulling the ones we are missing ==="
for f in libHeadImuCalibrate_int.so libHeadImuCalibrate.so \
         libpvrmodule_externalhmd.so libpvrmodule_controller.so \
         libpvrmodule_hmd.so libpvrmodule_eyetracker.so; do
  for d in lib64 lib; do
    if [ ! -f "$OUT/$d/$f" ]; then
      debugfs -R "dump /$d/$f $OUT/$d/$f" "$IMG" 2>/dev/null
      if [ -s "$OUT/$d/$f" ]; then
        printf '  %-42s %8d bytes\n' "$d/$f" "$(stat -c%s "$OUT/$d/$f")"
      else
        rm -f "$OUT/$d/$f"
      fi
    fi
  done
done
echo

# Which removed-in-Q framework symbols do the PVR blobs actually import?
# These are the ones that break a straight copy onto a newer framework.
echo "=== undefined framework symbols that Android 10 no longer provides ==="
GONE='getBuiltInDisplay|SurfaceComposerClient|ISurfaceComposer|setDisplayInfo|getDisplayInfo|IGraphicBufferProducer|GraphicBuffer'
for f in "$OUT"/lib64/*.so "$OUT"/lib/*.so; do
  [ -f "$f" ] || continue
  syms=$(readelf -W --dyn-syms "$f" 2>/dev/null | awk '$7=="UND"{print $8}' | grep -E "$GONE" | sort -u)
  if [ -n "$syms" ]; then
    echo "### $(basename "$(dirname "$f")")/$(basename "$f")"
    echo "$syms" | sed 's/^/    /'
  fi
done
echo

echo "=== specifically: who wants getBuiltInDisplay ==="
for f in "$OUT"/lib64/*.so "$OUT"/lib/*.so "$OUT"/bin/*; do
  [ -f "$f" ] || continue
  if readelf -W --dyn-syms "$f" 2>/dev/null | grep -q 'getBuiltInDisplay'; then
    echo "  $(basename "$(dirname "$f")")/$(basename "$f")"
  fi
done
echo DONE

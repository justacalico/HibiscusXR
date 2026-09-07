#!/bin/bash
# The JNI entry point itself may preserve x28 correctly while something it CALLS
# does not - x28 only has to be trashed once anywhere below the trampoline. So
# widen the scan to every function in the Pico libs, and confirm the 64-bit SDK
# even exports the entry points we saw in the 32-bit one.
set -u
D=/mnt/f/PN2Lineage/notes/lib64
PY=$HOME/.pn2venv/bin/python
LOG=/mnt/f/PN2Lineage/notes/162_scan_all.txt
exec >"$LOG" 2>&1

echo "=== does the 64-bit SDK export the JNI entry points? ==="
nm -D --defined-only "$D/libPvr_UnitySDK.so" 2>/dev/null | grep -c 'Java_' | sed 's/^/  Java_ exports: /'
nm -D --defined-only "$D/libPvr_UnitySDK.so" 2>/dev/null | grep -iE 'updateDisplayInfo|getLensInfo|getDisplayInfo|UpdateLensAndDisplay' | sed 's/^/  /'

echo
echo "=== ALL functions that write x28 without saving it ==="
for f in "$D"/*.so; do
  "$PY" /mnt/f/PN2Lineage/tools/160_find_x28.py "$f" --all --quiet
done
echo
echo DONE

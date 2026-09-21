#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Linear x28 sweep over everything loaded into the VRShell2 process, plus a list
# of the JNI entry points in the app's own copy of the SDK (the interposition
# target if we end up shimming the ABI violation).
set -u
PY=$HOME/.pn2venv/bin/python
LOG=${PN2_ROOT}/notes/170_sweep.txt
exec >"$LOG" 2>&1

echo "=== JNI entry points in the app's bundled SDK ==="
nm -D --defined-only ${PN2_ROOT}/notes/vrshell_lib/libPvr_UnitySDK.so 2>/dev/null |
  grep ' T Java_' | awk '{print "  "$3}' | sort
echo
echo "  (count: $(nm -D --defined-only ${PN2_ROOT}/notes/vrshell_lib/libPvr_UnitySDK.so 2>/dev/null | grep -c ' T Java_'))"

echo
echo "=== linear x28 sweep: app libs ==="
for f in ${PN2_ROOT}/notes/vrshell_lib/*.so; do
  "$PY" ${PN2_ROOT}/tools/169_linear_x28.py "$f"
done

echo
echo "=== linear x28 sweep: system Pico libs ==="
for f in ${PN2_ROOT}/notes/lib64/*.so; do
  "$PY" ${PN2_ROOT}/tools/169_linear_x28.py "$f"
done
echo
echo DONE

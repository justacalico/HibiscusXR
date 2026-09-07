#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# VRShell dlopens libpxr_6dof_optimization.so then dlsyms updateOffsets and
# getResetPos, and both fail. Find which library really exports them - across the
# whole stock system image, not just what we happened to extract.
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
LOG=${PN2_ROOT}/notes/111_syms.txt
exec >"$LOG" 2>&1

echo "=== in what we already extracted ==="
for d in ${PN2_ROOT}/pvr_stack ${PN2_ROOT}/pvr_applibs; do
  find "$d" -name '*.so' 2>/dev/null | while read -r f; do
    if nm -D --defined-only "$f" 2>/dev/null | grep -qE ' (updateOffsets|getResetPos)$'; then
      echo "  HIT $f"
      nm -D --defined-only "$f" 2>/dev/null | grep -E ' (updateOffsets|getResetPos)$' | sed 's/^/       /'
    fi
  done
done
echo

echo "=== does OUR copy of libpxr_6dof_optimization.so have them at all? ==="
for f in ${PN2_ROOT}/pvr_stack/lib64/libpxr_6dof_optimization.so \
         ${PN2_ROOT}/pvr_stack/lib/libpxr_6dof_optimization.so; do
  [ -f "$f" ] || continue
  echo "--- $f ---"
  echo "    total dynamic symbols: $(nm -D "$f" 2>/dev/null | wc -l)"
  nm -D --defined-only "$f" 2>/dev/null | grep -iE 'offset|reset' | head -10
done
echo

echo "=== brute force: which /lib64 or /lib file in the image contains the strings ==="
for d in lib64 lib; do
  for f in $(debugfs -R "ls -l /$d" "$IMG" 2>/dev/null | awk '{print $NF}' | grep '\.so$'); do
    tmp=/tmp/probe.so
    debugfs -R "dump /$d/$f $tmp" "$IMG" 2>/dev/null
    [ -s "$tmp" ] || continue
    if strings -a "$tmp" 2>/dev/null | grep -qx 'updateOffsets'; then
      echo "  $d/$f contains 'updateOffsets'"
    fi
    rm -f "$tmp"
  done
done
echo DONE

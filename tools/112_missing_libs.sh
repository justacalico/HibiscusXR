#!/bin/bash
# lib6DofReset.so exports updateOffsets and I never extracted it. My lib list was
# hand-written from a grep of the directory listing, so anything not matching
# pvr|pxr|qvr|pico was invisible - lib6DofReset.so is exactly that.
#
# Rather than keep discovering these one crash at a time, diff the whole stock
# /lib{,64} against what we have installed and pull anything Pico-ish that is
# missing.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
OUT=/mnt/f/PN2Lineage/pvr_stack
LOG=/mnt/f/PN2Lineage/notes/112_missing.txt
exec >"$LOG" 2>&1

# libs that exist in the stock image but are NOT part of stock AOSP - anything
# with these markers is Pico/Qualcomm-specific and a candidate dependency
PAT='6Dof|SixDof|pvr|pxr|qvr|pico|psmart|svr|vraudio|2dToVr|tracking|CVCon|NDI|Imu|IMU|Calibrat'

for d in lib64 lib; do
  echo "=== /$d ==="
  for f in $(debugfs -R "ls -l /$d" "$IMG" 2>/dev/null | awk '{print $NF}' | grep '\.so$' | grep -iE "$PAT"); do
    if [ -f "$OUT/$d/$f" ]; then
      continue
    fi
    debugfs -R "dump /$d/$f $OUT/$d/$f" "$IMG" 2>/dev/null
    if [ -s "$OUT/$d/$f" ]; then
      printf '  PULLED %-38s %10d\n' "$d/$f" "$(stat -c%s "$OUT/$d/$f")"
    else
      rm -f "$OUT/$d/$f"
    fi
  done
done

echo
echo "=== does lib6DofReset.so really export what VRShell wants ==="
for a in lib64 lib; do
  f="$OUT/$a/lib6DofReset.so"
  [ -f "$f" ] || continue
  echo "--- $a ---"
  nm -D --defined-only "$f" 2>/dev/null | grep -E 'updateOffsets|getResetPos|reset' | head -10
done
echo DONE

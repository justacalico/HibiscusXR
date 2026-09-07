#!/bin/bash
# Add the DSP tracking stack to system-pn2-full.img.
#
# This is the session's biggest find. Android 8.1 let /system processes load from
# /vendor/lib; Android 10's namespace separation does not, and the GSI never
# carried the DSP-side files at all. Three pieces were missing:
#
#   libcdsprpc.so  - Compute DSP RPC (the /vendor copy: its SONAME matches the
#                    stub's verneed; the _system variant is rejected on symbol
#                    versioning)
#   libmdsprpc.so  - Modem DSP RPC, needed by the mapper wrapper
#   /system/lib/rfsa/adsp/* - the DSP-SIDE libraries FastRPC loads onto the DSP,
#                    including libtracker_6dof_skel.so (21MB) and
#                    libVIOMapping_6dof_skel.so (16MB) - the actual SLAM code
#
# Verified: qvrservicetest64 now prints "starting VR mode" with live gyro/accel,
# matching stock, where it previously said "VR not supported".
set -u
IMG=/mnt/f/PN2Lineage/out/system-pn2-full.img
LOG=/mnt/f/PN2Lineage/notes/300_img_dsp.txt
exec >"$LOG" 2>&1

fail=0
put() {
  local src="$1" dst="$2" mode="$3" dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  [ -f "$src" ] || { printf '  MISSING SRC %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then printf '  OK    %-46s %12s\n' "$dst" "$got"
  else printf '  FAIL  %-46s want %s got "%s"\n' "$dst" "$want" "${got:-absent}"; fail=$((fail+1)); fi
}
mkd() {
  debugfs -w -R "mkdir $1" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 mode 040755" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 gid 0" "$IMG" >/dev/null 2>&1
}

echo "=== free before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

echo
echo "=== DSP RPC libraries (vendor copies - correct SONAME for the verneed) ==="
put /mnt/f/PN2Lineage/cdsp/vendor/libcdsprpc.so /lib/libcdsprpc.so 644

echo
echo "=== DSP-side skel libraries ==="
mkd /lib/rfsa
mkd /lib/rfsa/adsp
for f in /mnt/f/PN2Lineage/rfsa/*.so; do
  put "$f" "/lib/rfsa/adsp/$(basename "$f")" 644
done

echo
echo "=== repair + verify ==="
e2fsck -fy "$IMG" 2>&1 | tail -4
if e2fsck -fn "$IMG" >/tmp/f.txt 2>&1; then tail -2 /tmp/f.txt; echo "  CLEAN"; else tail -6 /tmp/f.txt; echo "  DIRTY"; fail=$((fail+1)); fi

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "DSP STACK ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

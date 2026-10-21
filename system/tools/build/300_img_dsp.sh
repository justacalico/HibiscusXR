#!/bin/bash
HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
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
IMG=${HIBISCUS_ROOT}/out/system-pn2-full.img
LOG=${HIBISCUS_ROOT}/notes/300_img_dsp.txt
exec >"$LOG" 2>&1

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo "=== free before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

echo
echo "=== DSP RPC libraries (vendor copies - correct SONAME for the verneed) ==="
put ${HIBISCUS_ROOT}/cdsp/vendor/libcdsprpc.so /lib/libcdsprpc.so 644

echo
echo "=== DSP-side skel libraries ==="
mkd /lib/rfsa
mkd /lib/rfsa/adsp
for f in ${HIBISCUS_ROOT}/rfsa/*.so; do
  put "$f" "/lib/rfsa/adsp/$(basename "$f")" 644
done

echo
echo "=== repair + verify ==="
img_fsck || fail=$((fail+1))

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "DSP STACK ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

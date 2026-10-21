#!/bin/bash
HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
# Add libmdsprpc.so (modem DSP RPC, needed by the QVR mapper wrapper) to the image.
# The /vendor copy is the one to use - its SONAME matches the stub's verneed.
set -u
IMG=${HIBISCUS_ROOT}/out/system-pn2-full.img
S=${HIBISCUS_ROOT}/cdsp/vendor/libmdsprpc.so
D=/lib/libmdsprpc.so

[ -f "$S" ] || { echo "source missing"; exit 1; }
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"
fail=0
put "$S" "$D" 644

img_fsck || fail=$((fail+1))

echo
echo "=== final DSP inventory in the image ==="
debugfs -R "ls -l /lib" "$IMG" 2>/dev/null | grep -E 'dsprpc' | awk '{printf "  %-24s %s\n", $NF, $6}'
echo "  rfsa/adsp: $(debugfs -R "ls /lib/rfsa/adsp" "$IMG" 2>/dev/null | tr ' ' '\n' | grep -c '\.so')"

#!/bin/bash
# Add libmdsprpc.so (modem DSP RPC, needed by the QVR mapper wrapper) to the image.
# The /vendor copy is the one to use - its SONAME matches the stub's verneed.
set -u
IMG=/mnt/f/PN2Lineage/out/system-pn2-full.img
S=/mnt/f/PN2Lineage/cdsp/vendor/libmdsprpc.so
D=/lib/libmdsprpc.so

[ -f "$S" ] || { echo "source missing"; exit 1; }
debugfs -w -R "rm $D"          "$IMG" >/dev/null 2>&1
debugfs -w -R "write $S $D"    "$IMG" >/dev/null 2>&1
debugfs -w -R "sif $D mode 0100644" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif $D uid 0"   "$IMG" >/dev/null 2>&1
debugfs -w -R "sif $D gid 0"   "$IMG" >/dev/null 2>&1

want=$(stat -c%s "$S")
got=$(debugfs -R "ls -l /lib" "$IMG" 2>/dev/null | awk '$NF=="libmdsprpc.so"{print $6}')
echo "  want $want, image says ${got:-absent}"
[ "$got" = "$want" ] && echo "  OK" || echo "  FAIL"

e2fsck -fy "$IMG" >/dev/null 2>&1
if e2fsck -fn "$IMG" >/dev/null 2>&1; then echo "  fsck CLEAN"; else echo "  fsck DIRTY"; fi

echo
echo "=== final DSP inventory in the image ==="
debugfs -R "ls -l /lib" "$IMG" 2>/dev/null | grep -E 'dsprpc' | awk '{printf "  %-24s %s\n", $NF, $6}'
echo "  rfsa/adsp: $(debugfs -R "ls /lib/rfsa/adsp" "$IMG" 2>/dev/null | tr ' ' '\n' | grep -c '\.so')"

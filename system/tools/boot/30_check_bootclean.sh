#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
S=${PN2_ROOT}/extracted/bootclean/rd
M=${PN2_ROOT}/extracted/bootpatch/rd
L=${PN2_ROOT}/notes/30_bootclean.log
exec >"$L" 2>&1

echo "=== OTA (stock) boot ramdisk ==="
ls -la "$S" 2>&1
echo
echo "=== magisk markers in OTA ramdisk ==="
found=0
for m in .backup overlay.d sbin init.magisk.rc magiskinit; do
  if [ -e "$S/$m" ]; then echo "PRESENT: $m"; found=1; fi
done
[ "$found" -eq 0 ] && echo "none -> OTA boot.img is CLEAN stock"
echo
echo "=== magisk markers in the on-device (backed up) boot ramdisk ==="
for m in .backup overlay.d sbin init.magisk.rc magiskinit; do
  if [ -e "$M/$m" ]; then echo "PRESENT: $m"; fi
done
echo
echo "=== init size comparison (magisk replaces init) ==="
printf 'OTA    init: %s bytes\n' "$(stat -c%s "$S/init" 2>/dev/null)"
printf 'device init: %s bytes\n' "$(stat -c%s "$M/init" 2>/dev/null)"
echo
echo "=== does OTA ramdisk also symlink default.prop? ==="
ls -l "$S/default.prop" 2>&1
echo DONE

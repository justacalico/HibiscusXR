#!/bin/bash
set -u
G=/mnt/f/PN2Lineage/gsi/lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img
L=/mnt/f/PN2Lineage/notes/22_gsi.log
exec >"$L" 2>&1

echo "=== identity ==="
file -b "$G"
SZ=$(stat -c%s "$G")
echo "gsi size:       $SZ bytes ($((SZ/1024/1024)) MB)"
echo "target system:  3943694336 bytes (3761 MB)"
if [ "$SZ" -le 3943694336 ]; then echo "FITS in system partition"; else echo "TOO BIG for system partition"; fi
echo

echo "=== root layout ==="
echo "(A-only GSI should contain /system/... i.e. NOT system-as-root)"
debugfs -R "ls -l /" "$G" 2>/dev/null | head -25
echo

echo "=== does it have a nested /system ? ==="
debugfs -R "ls -l /system" "$G" 2>/dev/null | head -15
echo

echo "=== build.prop ==="
for p in /system/build.prop /build.prop; do
  if debugfs -R "dump -p $p /tmp/gbp" "$G" >/dev/null 2>&1 && [ -s /tmp/gbp ]; then
    echo "found at $p"
    grep -E 'ro.build.version.release|ro.build.version.sdk|ro.lineage.version|ro.build.flavor|ro.product.system.name|ro.vndk.version|ro.treble' /tmp/gbp | head -15
    break
  fi
done
echo

echo "=== VNDK 27 support present? (vendor is VNDK 27) ==="
debugfs -R "ls /system/lib64/vndk-27" "$G" 2>/dev/null | tr ' ' '\n' | grep -c '\.so' || echo "no lib64/vndk-27 dir"
debugfs -R "ls /system/lib64/vndk-sp-27" "$G" 2>/dev/null | tr ' ' '\n' | grep -c '\.so' || echo "no lib64/vndk-sp-27 dir"
echo
echo "(a VNDK 27 snapshot in the GSI is what lets an Android 10 system talk to"
echo " an Android 8.1 vendor partition - this is the single most important check)"
echo DONE

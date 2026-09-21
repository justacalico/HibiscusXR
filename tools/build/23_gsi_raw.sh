#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
D=${PN2_ROOT}/gsi
G=$D/lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img
R=$D/gsi_raw.img
L=${PN2_ROOT}/notes/23_gsi_raw.log
exec >"$L" 2>&1

if [ ! -f "$R" ]; then
  echo "=== simg2img ==="
  simg2img "$G" "$R"
  echo "exit $?"
fi
ls -l "$R"
file -b "$R"
echo

echo "=== root layout ==="
debugfs -R "ls -l /" "$R" 2>/dev/null | head -30
echo

echo "=== build.prop ==="
for p in /system/build.prop /build.prop; do
  if debugfs -R "dump -p $p /tmp/gbp" "$R" >/dev/null 2>&1 && [ -s /tmp/gbp ]; then
    echo "--- found at $p ---"
    grep -E 'ro.build.version.release|ro.build.version.sdk|ro.lineage.version|ro.build.flavor|ro.vndk.version|ro.treble|ro.product.*name' /tmp/gbp | head -15
    break
  fi
done
echo

echo "=== VNDK snapshot dirs present in the GSI ==="
for d in /system/lib64 /lib64; do
  out=$(debugfs -R "ls $d" "$R" 2>/dev/null | tr ' ' '\n' | grep -E '^vndk' | sort -u)
  if [ -n "$out" ]; then echo "under $d:"; echo "$out" | sed 's/^/   /'; fi
done
echo

echo "=== CRITICAL: is VNDK 27 present? ==="
n27=$(debugfs -R "ls /system/lib64/vndk-27" "$R" 2>/dev/null | tr ' ' '\n' | grep -c '\.so')
s27=$(debugfs -R "ls /system/lib64/vndk-sp-27" "$R" 2>/dev/null | tr ' ' '\n' | grep -c '\.so')
echo "vndk-27    libs: $n27"
echo "vndk-sp-27 libs: $s27"
if [ "$n27" -gt 0 ]; then
  echo "RESULT: GSI ships a VNDK 27 snapshot - our VNDK 27 vendor should be able to bind."
else
  echo "RESULT: NO VNDK 27 snapshot. The Android 8.1 vendor partition will not"
  echo "        find its VNDK libs. Expect a boot loop or a dead vendor HAL layer."
fi
echo DONE

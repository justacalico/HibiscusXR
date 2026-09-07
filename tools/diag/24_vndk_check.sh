#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
R=${PN2_ROOT}/gsi/gsi_raw.img
L=${PN2_ROOT}/notes/24_vndk.log
exec >"$L" 2>&1

echo "NOTE: this GSI's root IS /system content, so VNDK lives at /lib64/vndk-NN"
echo

for d in vndk-26 vndk-27 vndk-28 vndk-29 vndk-sp-26 vndk-sp-27 vndk-sp-28 vndk-sp-29; do
  n=$(debugfs -R "ls /lib64/$d" "$R" 2>/dev/null | tr ' ' '\n' | grep -c 'so$')
  printf '/lib64/%-14s %4s libs\n' "$d" "$n"
done
echo
for d in vndk-27 vndk-sp-27; do
  n=$(debugfs -R "ls /lib/$d" "$R" 2>/dev/null | tr ' ' '\n' | grep -c 'so$')
  printf '/lib/%-16s %4s libs  (32-bit)\n' "$d" "$n"
done
echo

echo "=== sample of /lib64/vndk-27 ==="
debugfs -R "ls /lib64/vndk-27" "$R" 2>/dev/null | tr ' ' '\n' | grep 'so$' | head -15
echo

N=$(debugfs -R "ls /lib64/vndk-27" "$R" 2>/dev/null | tr ' ' '\n' | grep -c 'so$')
echo "=== VERDICT ==="
if [ "$N" -gt 20 ]; then
  echo "VNDK 27 snapshot present ($N libs)."
  echo "Our VNDK 27 vendor partition has what it needs. Boot is plausible."
else
  echo "VNDK 27 snapshot missing or too small ($N libs). Expect vendor HAL failure."
fi
echo DONE

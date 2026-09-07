#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
B=${PN2_ROOT}
P=$B/images/snapshot_lun0/persist.bin
N=$B/notes
W=/home/justin/pn2

{
echo "=== persist.bin identity ==="
file -b "$P"
echo
echo "=== persist root ==="
debugfs -R "ls -l /" "$P" 2>/dev/null
echo
echo "=== persist /pvr ==="
debugfs -R "ls -l /pvr" "$P" 2>/dev/null
echo
echo "=== persist /pvr/camera  (THE CALIBRATION) ==="
debugfs -R "ls -l /pvr/camera" "$P" 2>/dev/null
echo
echo "=== full persist dump ==="
rm -rf "$W/persist"; mkdir -p "$W/persist"
debugfs -R "rdump / $W/persist" "$P" >/dev/null 2>&1
find "$W/persist" -type f -printf '%10s  %P\n' | sort -k2
} > "$N/08a_persist.log" 2>&1

{
echo "=== calibration file previews ==="
for f in $(find "$W/persist" -type f | head -40); do
  echo "########## $f ($(stat -c%s "$f") bytes) ##########"
  if file -b "$f" | grep -qi text; then
    head -c 1200 "$f"
  else
    xxd "$f" | head -12
  fi
  echo; echo
done
} > "$N/08b_calib_preview.log" 2>&1

echo DONE
wc -l "$N"/08*.log

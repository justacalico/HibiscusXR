#!/bin/bash
# Our own headset's original persist partition, from the pre-conversion backup.
# The live device has lost 9 of the 10 files in /pvr/camera - only
# device_calibration.xml survived - and with no hwcalibration.merge the 6DoF
# tracker never gets a calibration, so pose quality sits at 0.00 and it
# relocalises forever. These are per-unit files, so they have to come from OUR
# backup, not from the stock reference headset (which is a different unit, and an
# Eye model at that).
set -u
IMG=/mnt/f/PN2Lineage/backup_nonEye/persist.img
WORK=/tmp/persist_ours.img
OUT=/mnt/f/PN2Lineage/persist_calib

cp -f "$IMG" "$WORK"
# the backup was taken from a live device, so the journal is dirty
e2fsck -fy "$WORK" >/dev/null 2>&1

echo "=== /pvr/camera in OUR backup ==="
debugfs -R "ls -l /pvr/camera" "$WORK" 2>/dev/null

echo
echo "=== /pvr/lens ==="
debugfs -R "ls -l /pvr/lens" "$WORK" 2>/dev/null

echo
echo "=== extracting ==="
mkdir -p "$OUT/camera" "$OUT/lens"
for f in base.merge camera0.merge camera1.merge device_calibration.xml \
         hwcalibration.merge leds.merge lens_config.merge platformtype.merge \
         product.merge xconfig.merge; do
  debugfs -R "dump /pvr/camera/$f $OUT/camera/$f" "$WORK" >/dev/null 2>&1
  if [ -s "$OUT/camera/$f" ]; then
    printf '  OK      %-28s %8s\n' "$f" "$(stat -c%s "$OUT/camera/$f")"
  else
    printf '  ABSENT  %s\n' "$f"; rm -f "$OUT/camera/$f"
  fi
done
debugfs -R "dump /pvr/lens/axisOffset.txt $OUT/lens/axisOffset.txt" "$WORK" >/dev/null 2>&1
[ -s "$OUT/lens/axisOffset.txt" ] && echo "  OK      axisOffset.txt"

echo
echo "=== does our backup identify the right unit? ==="
grep -o 'deviceUID="[0-9]*"' "$OUT/camera/device_calibration.xml" 2>/dev/null
echo "  (live device reports deviceUID=2218225873)"

echo
echo "=== hwcalibration.merge content ==="
cat "$OUT/camera/hwcalibration.merge" 2>/dev/null
echo
echo "=== product / platformtype ==="
cat "$OUT/camera/product.merge" 2>/dev/null
cat "$OUT/camera/platformtype.merge" 2>/dev/null
rm -f "$WORK"

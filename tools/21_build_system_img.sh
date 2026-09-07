#!/bin/bash
# Reconstruct stock system.img from the OTA so restore_stock.ps1 has something
# to flash. Without this the restore path is broken.
set -u
B=/mnt/f/PN2Lineage
OTA=$B/images/ota_4.1.3
ZIP='/mnt/f/Pico_Neo_Firmware/Neo_2/update_PicoNeo2_pui4.1.3_20210409_b346-user-jdi4k72_new.zip'
L=$B/notes/21_system_img.log
exec >"$L" 2>&1

echo "=== free space before ==="
df -h /mnt/f | tail -1

if [ -f "$OTA/system.img" ]; then
  echo "system.img already present:"
  ls -l "$OTA/system.img"
  exit 0
fi

echo "=== extracting system.new.dat.br ==="
unzip -o -q "$ZIP" 'system.new.dat.br' 'system.transfer.list' -d "$OTA"
ls -l "$OTA/system.new.dat.br" "$OTA/system.transfer.list"

echo "=== brotli decompress ==="
brotli -d -o "$OTA/system.new.dat" "$OTA/system.new.dat.br"
echo "exit $?"
rm -f "$OTA/system.new.dat.br"
ls -l "$OTA/system.new.dat"

echo "=== sdat2img ==="
python3 "$B/tools/03_sdat2img.py" "$OTA/system.transfer.list" "$OTA/system.new.dat" "$OTA/system.img"
rm -f "$OTA/system.new.dat"

echo "=== result ==="
ls -l "$OTA/system.img"
file -b "$OTA/system.img"
echo
echo "=== sanity: is it the stock Pico system? ==="
debugfs -R "ls -l /" "$OTA/system.img" 2>/dev/null | head -12
echo
debugfs -R "dump -p /build.prop /tmp/bp" "$OTA/system.img" 2>/dev/null
grep -E 'ro.build.fingerprint|ro.pvr.internal.version|ro.build.version.release' /tmp/bp 2>/dev/null

echo
echo "=== free space after ==="
df -h /mnt/f | tail -1
echo DONE

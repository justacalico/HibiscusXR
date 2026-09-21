#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Characterise the NDI electromagnetic controller tracking stack.
set -u
W=/home/justin/pn2
V=$W/vendor
N=${PN2_ROOT}/notes
SYS=${PN2_ROOT}/images/snapshot_lun0/system.bin

{
echo "############ NDI / EM / FPGA / controller named files ############"
echo "--- vendor ---"
find "$V" -iregex '.*\(ndi\|fpga\|emt\|magnet\|controller\|finch\|cv\).*' -printf '%10s  %p\n' 2>/dev/null | sed "s|$V|/vendor|" | sort -k2
echo
echo "--- persist ---"
find "$W/persist" -iregex '.*\(ndi\|fpga\|controller\|emt\|magnet\).*' -printf '%10s  %p\n' 2>/dev/null | sort -k2
echo
echo "--- system/lib64 (from image) ---"
debugfs -R "ls -l /lib64" "$SYS" 2>/dev/null | grep -iE 'ndi|fpga|controller|finch|magnet|emt'
echo
echo "--- system/bin (from image) ---"
debugfs -R "ls -l /bin" "$SYS" 2>/dev/null | grep -iE 'ndi|fpga|controller|finch|magnet|emt'
} > "$N/09a_ndi_files.log" 2>&1

{
echo "############ pxr.ndifirmware.update.sh ############"
cat "$V/etc/pvr/pxr.ndifirmware.update.sh"
echo
echo "############ pxr.vendorhw.check.sh ############"
cat "$V/etc/pvr/pxr.vendorhw.check.sh"
} > "$N/09b_ndi_scripts.log" 2>&1

{
echo "############ sensors-common-ndifpga.pvr.so ############"
P="$V/lib64/sensors-common-ndifpga.pvr.so"
file -b "$P"
echo "--- NEEDED ---"
readelf -d "$P" 2>/dev/null | grep NEEDED | sed 's/^ *//'
echo "--- exported FUNC ---"
readelf --dyn-syms -W "$P" 2>/dev/null | awk '$4=="FUNC" && $7!="UND" {print $8}' | sort -u | head -60
echo
echo "--- strings: device nodes / paths / firmware ---"
strings -n 5 "$P" | grep -iE '^/|\.bin$|\.hex$|fpga|ndi|spi|i2c|gpio|dev/' | sort -u | head -60
echo
echo "############ sensors-common-imu.pvr.so : paths ############"
strings -n 5 "$V/lib64/sensors-common-imu.pvr.so" | grep -iE '^/|dev/|icm|spi|i2c' | sort -u | head -40
echo
echo "############ sensors-icmxx.pvr.so : chip ids ############"
strings -n 4 "$V/lib64/sensors-icmxx.pvr.so" | grep -iE 'icm[0-9]|mpu[0-9]|invn|invensense' | sort -u | head -30
} > "$N/09c_ndi_elf.log" 2>&1

{
echo "############ /persist/ndi contents ############"
for f in "$W/persist/ndi"/*; do
  [ -f "$f" ] || continue
  echo "===== $f ($(stat -c%s "$f") bytes) ====="
  xxd "$f" | head -20
  echo
done
echo "############ /persist/vendorhw ############"
find "$W/persist/vendorhw" -type f -printf '%10s  %p\n' 2>/dev/null
for f in $(find "$W/persist/vendorhw" -type f 2>/dev/null | head -6); do
  echo "===== $f ====="
  head -c 400 "$f"; echo
done
} > "$N/09d_ndi_persist.log" 2>&1

echo DONE
wc -l "$N"/09*.log

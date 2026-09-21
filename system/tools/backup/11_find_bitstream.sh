#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Hunt the NDI FPGA bitstream. Start with the flasher binary: whatever path it opens is authoritative.
set -u
B=${PN2_ROOT}
FW=$B/ndi_firmware
W=/home/justin/pn2
N=$B/notes/11_bitstream_hunt.log
SYS=$B/images/snapshot_lun0/system.bin
VEN=$B/images/vendor.img
OEM=$B/images/ota_4.1.3/oem.bin
exec >"$N" 2>&1

TOOL=$FW/EYE_pui4.1.3_firehose/w25q_write_bin

echo "################ 1. w25q_write_bin : what does it open? ################"
file -b "$TOOL"
echo "--- NEEDED ---"
readelf -d "$TOOL" 2>/dev/null | grep NEEDED | sed 's/^ *//'
echo
echo "--- ALL strings (this binary is tiny; show everything) ---"
strings -n 3 "$TOOL" | sort -u
echo

echo "################ 2. /system/etc listing (is there an ndi dir at all?) ################"
debugfs -R "ls -l /etc" "$SYS" 2>/dev/null | grep -iE 'ndi|fpga|bitmap|firmware'
echo "(full /system/etc entry count:)"
debugfs -R "ls /etc" "$SYS" 2>/dev/null | tr ' ' '\n' | grep -c .
echo

echo "################ 3. filename string search across images ################"
for img in "$SYS:system" "$VEN:vendor" "$OEM:oem"; do
  p=${img%%:*}; lbl=${img##*:}
  printf '%-8s ' "$lbl"
  if grep -a -c -o 'top_level_bitmap' "$p" 2>/dev/null | head -1 | grep -qv '^0$'; then
    echo "HIT ($(grep -a -o -c 'top_level_bitmap' "$p" 2>/dev/null) occurrences)"
    grep -a -o -b 'top_level_bitmap[^ ]\{0,40\}' "$p" 2>/dev/null | head -5
  else
    echo "no 'top_level_bitmap' string"
  fi
done
echo

echo "################ 4. any *.bin under vendor/etc + firmware dirs ################"
find "$W/vendor" -name '*.bin' -printf '%10s  %p\n' 2>/dev/null | sed "s|$W/vendor|/vendor|" | sort -k2 | head -60
echo

echo "################ 5. oem.bin deep listing ################"
debugfs -R "ls -l /" "$OEM" 2>/dev/null
echo "--- oem priv-app ---"
debugfs -R "ls -l /priv-app" "$OEM" 2>/dev/null | head -40
echo

echo "################ 6. search all OTA zips for ndi/bitmap entries ################"
for z in ${PICO_FW_DIR}/Neo_2/*.zip ${PICO_FW_DIR}/G2/*.zip; do
  echo "--- $(basename "$z") ---"
  unzip -l "$z" 2>/dev/null | grep -iE 'ndi|fpga|bitmap|w25q' || echo "  (none)"
done
echo

echo "################ 7. Xilinx/Lattice/Altera bitstream magic scan ################"
# Lattice .bin often starts with 0xFF00; Xilinx bitstreams contain 'ffffffffaa995566' sync word
for p in "$SYS" "$VEN" "$OEM"; do
  printf '%-56s ' "$p"
  if grep -a -q -o 'aa995566\|Vivado\|ISE Webpack\|LATTICE\|iCEcube\|Quartus\|xc7\|LFE5\|ice40' "$p" 2>/dev/null; then
    echo "possible FPGA toolchain marker"
    grep -a -o 'Vivado[^ ]\{0,30\}\|LATTICE[^ ]\{0,20\}\|Quartus[^ ]\{0,20\}\|ice40[^ ]\{0,10\}' "$p" 2>/dev/null | sort -u | head -10
  else
    echo "no FPGA toolchain marker"
  fi
done

echo DONE

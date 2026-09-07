#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Recover the NDI FPGA bitstream (top_level_bitmap.bin) from every available firmware source.
# Rebuilds each OTA system image one at a time and deletes it after, to stay inside the disk budget.
set -u
B=${PN2_ROOT}
FW=$B/ndi_firmware
IMG=$B/images
FWDIR=${PICO_FW_DIR}/Neo_2
N=$B/notes/10_ndi_firmware.log
TMP=$IMG/_tmp_sys

mkdir -p "$FW"
exec >"$N" 2>&1

echo "=== free space before ==="
df -h "${PN2_ROOT:-$HOME/PN2Lineage}" | tail -1
echo

pull_ndi() {   # image  label
  local img="$1"
  local label="$2"
  local out="$FW/$label"
  mkdir -p "$out"
  echo "--- $label : /system/etc/ndi ---"
  debugfs -R "ls -l /etc/ndi" "$img" 2>/dev/null
  debugfs -R "rdump /etc/ndi $out" "$img" >/dev/null 2>&1
  # the FPGA flashing tool too
  debugfs -R "dump -p /bin/w25q_write_bin $out/w25q_write_bin" "$img" >/dev/null 2>&1
  find "$out" -type f -printf '%10s  %P\n' | sort -k2
  echo
}

# ---- 1. Eye variant, from the firehose dump ----
pull_ndi "$IMG/snapshot_lun0/system.bin" "EYE_pui4.1.3_firehose"

# ---- 2..4. each official non-Eye OTA ----
for z in "$FWDIR"/update_PicoNeo2_*.zip; do
  base=$(basename "$z" .zip)
  # short label: pui version + build
  label=$(echo "$base" | sed -E 's/update_PicoNeo2_(pui[0-9.]+)_([0-9]+)_(b[0-9]+).*/NONEYE_\1_\3/')
  echo "################ $label ################"
  echo "source: $base"

  rm -rf "$TMP"; mkdir -p "$TMP"
  unzip -o -q "$z" 'system.new.dat.br' 'system.transfer.list' -d "$TMP" 2>/dev/null
  if [ ! -f "$TMP/system.new.dat.br" ]; then
    echo "  no system.new.dat.br in this zip; skipping"
    rm -rf "$TMP"; continue
  fi
  brotli -d -o "$TMP/system.new.dat" "$TMP/system.new.dat.br" && rm -f "$TMP/system.new.dat.br"
  python3 "$B/tools/03_sdat2img.py" "$TMP/system.transfer.list" "$TMP/system.new.dat" "$TMP/system.img" \
    && rm -f "$TMP/system.new.dat"
  pull_ndi "$TMP/system.img" "$label"
  rm -rf "$TMP"
  echo "free: $(df -h "${PN2_ROOT:-$HOME/PN2Lineage}" | tail -1 | awk '{print $4}')"
  echo
done

echo
echo "################ CHECKSUM COMPARISON ################"
find "$FW" -name 'top_level_bitmap.bin' -print0 | xargs -0 -r md5sum | sort
echo
echo "--- all recovered files ---"
find "$FW" -type f -printf '%10s  %P\n' | sort -k2
echo
echo "--- bitstream identity ---"
for f in $(find "$FW" -name 'top_level_bitmap.bin'); do
  echo "== $f =="
  file -b "$f"
  xxd "$f" | head -6
  echo
done

echo "=== free space after ==="
df -h "${PN2_ROOT:-$HOME/PN2Lineage}" | tail -1

echo
echo "################ STAGING TO J:\\C Desktop ################"
DEST="${NDI_OUT:-$HOME/PicoNeo2_NDI_FPGA_Recovery}"
mkdir -p "$DEST"
for d in "$FW"/*/; do
  lbl=$(basename "$d")
  [ -f "$d/ndi/top_level_bitmap.bin" ] || continue
  cp "$d/ndi/top_level_bitmap.bin" "$DEST/top_level_bitmap_${lbl}.bin"
done
# flashing tool: identical across builds, take one
find "$FW" -name w25q_write_bin | head -1 | xargs -r -I{} cp {} "$DEST/w25q_write_bin"
ls -la "$DEST"
md5sum "$DEST"/* 2>/dev/null
echo DONE

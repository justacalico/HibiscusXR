#!/bin/bash
set -u
B=/mnt/f/PN2Lineage
EX=$B/extracted
N=$B/notes
V=/home/justin/pn2/vendor

mkdir -p "$EX/dtb"

# ---------------- A: split dtbo + scan kernel ----------------
{
python3 "$B/tools/16_dtb.py" dtbo "$B/images/ota_4.1.3/dtbo.img" "$EX/dtb"
echo
python3 "$B/tools/16_dtb.py" scan "$EX/boot_ota/kernel.raw" "$EX/dtb"
echo
python3 "$B/tools/16_dtb.py" scan "$EX/boot_ota/kernel.bin" "$EX/dtb"
} > "$N/17a_dtb_split.log" 2>&1

# ---------------- B: decompile all + index ----------------
{
for f in "$EX/dtb"/*.dtb; do
  o="${f%.dtb}.dts"
  if dtc -I dtb -O dts -o "$o" "$f" 2>/dev/null; then
    echo "OK   $(basename "$o")  $(wc -l < "$o") lines"
  else
    echo "FAIL $(basename "$f")"
  fi
done
echo
echo "=== model / compatible of each ==="
for f in "$EX/dtb"/*.dts; do
  m=$(grep -m1 -E '^\s*model\s*=' "$f" 2>/dev/null | sed 's/.*= *//; s/;//')
  c=$(grep -m1 -E '^\s*compatible\s*=' "$f" 2>/dev/null | sed 's/.*= *//; s/;//')
  printf '%-28s model=%-38s compat=%s\n' "$(basename "$f")" "${m:-?}" "${c:-?}"
done
} > "$N/17b_dtb_decompile.log" 2>&1

# ---------------- C: display panel hunt ----------------
{
echo "############ DSI panel nodes across all DTS ############"
grep -l -iE 'jdi|dsi.*panel|panel.*dsi' "$EX/dtb"/*.dts 2>/dev/null
echo
echo "############ panel node names ############"
grep -h -oE '^\s*(dsi_[a-z0-9_]+|[a-z0-9_]*panel[a-z0-9_]*)[ :@{]' "$EX/dtb"/*.dts 2>/dev/null | tr -d ' :{@' | sort -u | head -40
echo
echo "############ JDI / 4k / timing properties ############"
grep -h -iE 'jdi|qcom,mdss-dsi-panel-(width|height|framerate)|qcom,mdss-dsi-h-|qcom,mdss-dsi-v-|qcom,mdss-dsi-panel-timings|qcom,mdss-dsi-bpp|qcom,mdss-dsi-panel-clockrate|qcom,mdss-dsi-panel-name' \
  "$EX/dtb"/*.dts 2>/dev/null | sed 's/^\s*//' | sort -u | head -60
} > "$N/17c_panel.log" 2>&1

# ---------------- D: sensors / cameras / fpga ----------------
{
echo "############ camera sensor nodes ############"
grep -h -iE 'ov9282|ov6211|qcom,camera|cam_sensor|camera_module' "$EX/dtb"/*.dts 2>/dev/null | sed 's/^\s*//' | sort -u | head -40
echo
echo "############ IMU / icm ############"
grep -h -iE 'icm2|invensense|inven|imu' "$EX/dtb"/*.dts 2>/dev/null | sed 's/^\s*//' | sort -u | head -30
echo
echo "############ picovr / fpga / w25q ############"
grep -h -iE 'picovr|fpga|w25q|spidev' "$EX/dtb"/*.dts 2>/dev/null | sed 's/^\s*//' | sort -u | head -40
} > "$N/17d_hw.log" 2>&1

# ---------------- E: QVR client interface surface ----------------
{
for lib in libqvrservice_client.so libqvrcamera_client.so; do
  for arch in lib64 lib; do
    p="$V/$arch/$lib"
    [ -f "$p" ] || continue
    echo "################ $arch/$lib ################"
    echo "--- NEEDED ---"
    readelf -d "$p" 2>/dev/null | grep NEEDED | sed 's/^ *//'
    echo "--- exported functions ---"
    readelf --dyn-syms -W "$p" 2>/dev/null | awk '$4=="FUNC" && $7!="UND" {print $8}' | sort -u
    echo "--- count ---"
    readelf --dyn-syms -W "$p" 2>/dev/null | awk '$4=="FUNC" && $7!="UND" {print $8}' | sort -u | wc -l
    echo
  done
done
} > "$N/17e_qvr_iface.log" 2>&1

echo DONE
wc -l "$N"/17*.log

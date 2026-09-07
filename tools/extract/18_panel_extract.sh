#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
D=${PN2_ROOT}/extracted/dtb
N=${PN2_ROOT}/notes

# Which DTS carries the 72Hz JDI panel definition (not just a pinctrl reference)?
SRC=$(grep -l 'mdss_dsi_jdi_4k_55uhd_72_new_video {' "$D"/*.dts 2>/dev/null | head -1)
[ -z "$SRC" ] && SRC=$(grep -l 'jdi_4k_55uhd_72' "$D"/*.dts 2>/dev/null | head -1)

{
echo "############ source: $(basename "${SRC:-none}") ############"
echo
for P in jdi_4k_55uhd_72_new_video jdi_4k_55uhd_75_new_video jdi_4k_55uhd_70_new_video jdi_4k_55uhd_video; do
  echo "================================================================"
  echo "PANEL: $P"
  echo "================================================================"
  awk -v pat="mdss_dsi_${P} \\{" '
    $0 ~ pat {inb=1; depth=0}
    inb {
      print
      n=gsub(/\{/,"{"); depth+=n
      m=gsub(/\}/,"}"); depth-=m
      if (depth<=0 && NR>1 && n+m>0) {inb=0; print ""}
    }' "$SRC" 2>/dev/null | grep -vE 'qcom,mdss-dsi-(on|off)-command' | head -80
  echo
done
} > "$N/18a_panel_timings.log" 2>&1

{
echo "############ all JDI panel names + key timing props ############"
grep -hA200 'mdss_dsi_jdi_4k_55uhd_72_new_video {' "$SRC" 2>/dev/null | \
  grep -E 'panel-name|panel-width|panel-height|h-front-porch|h-back-porch|h-pulse-width|v-front-porch|v-back-porch|v-pulse-width|panel-framerate|bpp|traffic-mode|lane-map|t-clk|panel-clockrate|dsi-lane|panel-type|border' | head -40
echo
echo "############ Pico-specific device nodes (full) ############"
for NODE in 'spidev@0' 'icm@68' 'eepromi2c@57' 'nq@28' 'gpio_fan' 'hw_version' 'gpio_keys'; do
  echo "===== $NODE ====="
  awk -v pat="$NODE \\{" '
    $0 ~ pat {inb=1; depth=0}
    inb {
      print
      n=gsub(/\{/,"{"); depth+=n
      m=gsub(/\}/,"}"); depth-=m
      if (depth<=0 && n+m>0) {inb=0; print ""}
    }' "$SRC" 2>/dev/null | head -40
  echo
done
} > "$N/18b_pico_nodes.log" 2>&1

echo "SRC=$SRC"
wc -l "$N"/18*.log

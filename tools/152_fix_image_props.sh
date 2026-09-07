#!/bin/bash
# Repair build.prop inside BOTH images.
#
# The image builder appended to build.prop without guaranteeing a trailing
# newline, so lines got glued:
#     ro.sf.hwrotation=90ro.product.device=PICOA7B10
# which silently dropped ro.product.device. The device showed it after flashing;
# both images carry it.
#
# Rewrite the block properly: strip every key we manage, then append them each on
# their own line, after making sure the file ends with a newline.
set -u
LOG=/mnt/f/PN2Lineage/notes/152_props.txt
exec >"$LOG" 2>&1

fix_image() {
  local IMG="$1"
  echo "################ $(basename "$IMG") ################"
  [ -f "$IMG" ] || { echo "  missing"; return; }
  local T
  T=$(mktemp -d)
  debugfs -R "dump /build.prop $T/bp" "$IMG" 2>/dev/null
  if [ ! -s "$T/bp" ]; then echo "  could not read build.prop"; return; fi
  echo "  before: $(stat -c%s "$T/bp") bytes"
  echo "  glued lines before:"
  grep -nE '^[a-z].*=.*[a-z]+\.[a-z].*=' "$T/bp" | sed 's/^/    /' || echo "    none"

  # remove every key we manage, in any form we may have written it
  sed -i -E '/^#?(REVERTED )?ro\.surface_flinger\.primary_display_orientation/d' "$T/bp"
  sed -i -E '/^#?(REVERTED )?ro\.sf\.hwrotation/d' "$T/bp"
  sed -i -E '/^ro\.product\.(device|model|name|brand|manufacturer)=/d' "$T/bp"
  sed -i -E '/^persist\.bluetooth\.a2dp_offload\.disabled=/d' "$T/bp"
  sed -i -E '/^ro\.bluetooth\.a2dp_offload\.supported=/d' "$T/bp"
  sed -i -E '/Pico Neo 2 \(A7B10\) port additions/,+8d' "$T/bp"

  # guarantee a trailing newline before appending - this is the actual bug
  [ -n "$(tail -c1 "$T/bp")" ] && echo "" >> "$T/bp"

  cat >> "$T/bp" <<'EOF'

# --- Pico Neo 2 (A7B10) port additions -------------------------------------
# Vendor advertises persist.vendor.bt.a2dp_offload_cap but exposes no offload
# formats, so AudioPolicyManager null-derefs and audioserver crash-loops.
persist.bluetooth.a2dp_offload.disabled=true
ro.bluetooth.a2dp_offload.supported=false

# Panel reports 2160x3840 portrait but is mounted landscape across both eyes.
# Without this the 2D UI renders across the seam between the lenses. Verified
# NOT to affect Pico's VR stack: with native portrait, pvrservice logs identical
# lens/display values and VR apps still fail for an unrelated reason.
ro.surface_flinger.primary_display_orientation=ORIENTATION_90
ro.sf.hwrotation=90

# Device identity. pvrservice and friends read these.
ro.product.device=PICOA7B10
ro.product.model=Pico Neo 2
EOF

  sed -i -E 's/^ro\.build\.product=.*/ro.build.product=PICOA7B10/' "$T/bp"

  echo "  after : $(stat -c%s "$T/bp") bytes"
  echo "  glued lines after:"
  grep -nE '^[a-z].*=.*[a-z]+\.[a-z].*=' "$T/bp" | sed 's/^/    /' || echo "    none - clean"

  debugfs -w -R "rm /build.prop" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $T/bp /build.prop" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif /build.prop mode 0100600" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif /build.prop uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif /build.prop gid 0" "$IMG" >/dev/null 2>&1

  local got want
  want=$(stat -c%s "$T/bp")
  got=$(debugfs -R "ls -l /" "$IMG" 2>/dev/null | awk '$NF=="build.prop" {print $6}')
  if [ "$got" = "$want" ]; then echo "  written OK ($got bytes)"; else echo "  WRITE FAILED (want $want got ${got:-absent})"; fi

  e2fsck -fy "$IMG" >/dev/null 2>&1
  if e2fsck -fn "$IMG" >/dev/null 2>&1; then echo "  fsck CLEAN"; else echo "  fsck DIRTY"; fi

  echo "  --- the managed keys as they now read ---"
  debugfs -R "dump /build.prop $T/verify" "$IMG" 2>/dev/null
  grep -nE '^(ro\.build\.product|ro\.product\.(device|model)|ro\.sf\.hwrotation|ro\.surface_flinger\.primary_display_orientation|persist\.bluetooth\.a2dp)' "$T/verify" | sed 's/^/    /'
  echo
}

fix_image /mnt/f/PN2Lineage/out/system-pn2.img
fix_image /mnt/f/PN2Lineage/out/system-pn2-full.img
echo DONE

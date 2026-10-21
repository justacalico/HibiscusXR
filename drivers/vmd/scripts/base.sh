#!/usr/bin/env bash
# vmd base payload - runs as drivers/vmd/driver.json's base step on the
# shared clean image. Stamps a neutral guest identity so the vmd image
# never reports another device's product/model (the pn2 driver detects on
# exactly those props).
set -u
R="${HIBISCUS_ROOT:?set HIBISCUS_ROOT to a build root - see system/dist/README.md}"
IMG="$R/out/system-base.img"
LOG="$R/notes/149_vmd_base.txt"
exec >"$LOG" 2>&1

fail=0
. "$R/tools/lib/img.sh"
[ -f "$IMG" ] || { echo "FAIL: $IMG missing - 143 never ran"; exit 1; }

TMP=$(mktemp -d)
debugfs -R "dump /build.prop $TMP/build.prop" "$IMG" 2>/dev/null
if [ -s "$TMP/build.prop" ]; then
  sed -i 's/^ro\.build\.product=.*/ro.build.product=vmd/' "$TMP/build.prop"
  grep -q '^ro.product.device=' "$TMP/build.prop" || echo 'ro.product.device=vmd' >> "$TMP/build.prop"
  grep -q '^ro.product.model='  "$TMP/build.prop" || echo 'ro.product.model=Hibiscus vmd' >> "$TMP/build.prop"
  put "$TMP/build.prop" /build.prop 600
else
  echo "  ERROR: could not read build.prop"
  fail=$((fail+1))
fi
rm -rf "$TMP"

img_fsck || fail=$((fail+1))
[ "$fail" -eq 0 ] && echo "VMD BASE OK" || echo "VMD BASE HAD $fail FAILURES"

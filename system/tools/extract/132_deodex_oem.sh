#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Unquicken the /oem apps' vdex, same as the /system ones.
VDEX=/home/justin/vdextools/vdexExtractor/bin/vdexExtractor
SRC=${PN2_ROOT}/oem_apps
OUT=${PN2_ROOT}/oem_dex
LOG=${PN2_ROOT}/notes/132_oem_dex.txt
exec >"$LOG" 2>&1

rm -rf "$OUT"; mkdir -p "$OUT"
find "$SRC" -name '*.vdex' | sort | while read -r v; do
  # .../<App>/oat/<arch>/<Name>.vdex
  app=$(basename "$(dirname "$(dirname "$(dirname "$v")")")")
  d="$OUT/$app"
  mkdir -p "$d"
  "$VDEX" -i "$v" -o "$d" -f --ignore-crc-error >"$d/_extract.log" 2>&1
  n=$(find "$d" -name '*.dex' | wc -l)
  printf '  %-18s dex=%s  %s\n' "$app" "$n" "$(du -sh "$d" | cut -f1)"
done
echo
echo "=== extracted ==="
find "$OUT" -name '*.dex' -printf '%-60p %10s\n' | sed "s|$OUT/||"
echo DONE

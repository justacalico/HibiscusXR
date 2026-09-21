#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Unquicken every Pico vdex and write plain dex out.
#
# vdexExtractor --unquicken walks each code item and reverts the *-quick opcodes
# back to normal ones using the quickening info stream, which is the part we
# could not do by just lifting bytes out of the vdex.
VDEX=/home/justin/vdextools/vdexExtractor/bin/vdexExtractor
SRC=${PN2_ROOT}/pvr_apps
OUT=${PN2_ROOT}/pvr_dex
LOG=${PN2_ROOT}/notes/95_deodex.txt
exec >"$LOG" 2>&1

echo "=== usage ==="
"$VDEX" 2>&1 | head -30
echo

rm -rf "$OUT"; mkdir -p "$OUT"

echo "=== unquickening ==="
find "$SRC" -name '*.vdex' | sort | while read -r v; do
  # .../<AppDir>/oat/<arch>/<Name>.vdex  -> app dir name
  app=$(basename "$(dirname "$(dirname "$(dirname "$v")")")")
  d="$OUT/$app"
  mkdir -p "$d"
  # unquickening is the DEFAULT (the flag is --no-unquicken to turn it OFF).
  # --ignore-crc-error because the decompiled dex no longer matches the checksum
  # recorded in the vdex, which is expected once the bytecode is rewritten.
  "$VDEX" -i "$v" -o "$d" -f --ignore-crc-error >"$d/_extract.log" 2>&1
  rc=$?
  n=$(find "$d" -name '*.dex' -o -name '*_classes*' -o -name '*.cdex' | wc -l)
  sz=$(du -sh "$d" 2>/dev/null | cut -f1)
  printf '  %-24s rc=%s files=%s size=%s\n' "$app" "$rc" "$n" "$sz"
done
echo

echo "=== what landed ==="
find "$OUT" -type f ! -name '_extract.log' -printf '%-70p %10s\n' | sed "s|$OUT/||"
echo

echo "=== a sample extract log ==="
cat "$OUT/pvrdisplay/_extract.log" 2>/dev/null | tail -20
echo DONE

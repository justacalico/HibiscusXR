#!/bin/bash
# 32-bit dependency closure for the whitelist. zygote (32-bit) preloads the same
# list, so a missing 32-bit dependency aborts boot exactly like the 64-bit case.
set -u
LOG=/mnt/f/PN2Lineage/notes/197_closure32.txt
exec >"$LOG" 2>&1
INV=/tmp/pn2_inventory.txt
tr -d '\r' < /mnt/f/PN2Lineage/notes/192_inventory.txt > "$INV"
D=/mnt/f/PN2Lineage/notes/lib32
needed() { readelf -dW "$1" 2>/dev/null | awk '/NEEDED/{gsub(/[\[\]]/,"",$5); print $5}'; }

bad=0
for f in "$D"/*.so; do
  [ -f "$f" ] || continue
  l=$(basename "$f")
  miss=""
  for n in $(needed "$f"); do
    grep -qx "$n" "$INV" || miss="$miss $n"
  done
  if [ -z "$miss" ]; then echo "  OK $l"; else echo "  XX $l  MISSING:$miss"; bad=$((bad+1)); fi
done
echo
if [ "$bad" -eq 0 ]; then echo "ALL 32-BIT DEPENDENCIES SATISFIED"; else echo "$bad LIBS WOULD ABORT ZYGOTE32"; fi
echo DONE

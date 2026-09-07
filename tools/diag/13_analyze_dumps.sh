#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
cd ${PN2_ROOT}/deadunit
L=${PN2_ROOT}/notes/13_deadunit.log
exec >"$L" 2>&1

for f in d8k.bin rb.bin dfull.bin; do
  echo "########## $f ##########"
  ls -l "$f"
  echo "--- distinct byte values present ---"
  od -An -v -t x1 "$f" | tr ' ' '\n' | grep -v '^$' | sort -u | tr '\n' ' '
  echo
  echo "--- byte histogram (top 6) ---"
  od -An -v -t x1 "$f" | tr ' ' '\n' | grep -v '^$' | sort | uniq -c | sort -rn | head -6
  echo "--- head 128 ---"
  xxd -l 128 "$f"
  echo "--- tail 64 ---"
  xxd -s -64 "$f"
  echo
done

echo "########## controller firmware (mybin) ##########"
for f in mybin/*; do
  echo "== $f ($(stat -c%s "$f") bytes) =="
  file -b "$f"
  xxd -l 96 "$f"
  echo "--- printable strings ---"
  strings -n 6 "$f" | head -10
  echo
done
echo DONE

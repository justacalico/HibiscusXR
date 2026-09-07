#!/bin/bash
# /bin/vr and /framework/vr.jar exist in the GSI already, and debugfs `write`
# does not overwrite - it fails and leaves the original. Remove then write.
set -u
FULL=/mnt/f/PN2Lineage/out/system-pn2-full.img
STAGE=/mnt/f/PN2Lineage/fullstage
LOG=/mnt/f/PN2Lineage/notes/146_fix.txt
exec >"$LOG" 2>&1

fix() {  # fix <img-path> <mode>
  local dst="$1" mode="$2" src="$STAGE${1}"
  [ -f "$src" ] || { echo "  no staged source for $dst"; return 1; }
  echo "  before: $(debugfs -R "ls -l $(dirname $dst)" "$FULL" 2>/dev/null | awk -v b="$(basename $dst)" '$NF==b {print $6}')"
  debugfs -w -R "rm $dst" "$FULL" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode $mode" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$FULL" >/dev/null 2>&1
  local got want
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $(dirname $dst)" "$FULL" 2>/dev/null | awk -v b="$(basename $dst)" '$NF==b {print $6}')
  if [ "$got" = "$want" ]; then echo "  OK   $dst -> $got"; else echo "  FAIL $dst want $want got $got"; fi
}

echo "=== /bin/vr ==="
fix /bin/vr 0100755
echo "=== /framework/vr.jar ==="
fix /framework/vr.jar 0100644

echo
echo "=== repair + verify ==="
e2fsck -fy "$FULL" >/dev/null 2>&1
if e2fsck -fn "$FULL" >/tmp/f.txt 2>&1; then tail -2 /tmp/f.txt; echo "  CLEAN"; else tail -6 /tmp/f.txt; echo "  DIRTY"; fi
ls -l "$FULL"
echo DONE

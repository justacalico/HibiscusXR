#!/bin/bash
# What condition makes PVRLauncher start Provision? Pull the candidate strings out
# of its dex - settings keys, property names and flags it might consult.
set -u
D=/mnt/f/PN2Lineage/notes/plx
LOG=/mnt/f/PN2Lineage/notes/254_launcher_strings.txt
exec >"$LOG" 2>&1

echo "=== provision-related strings ==="
strings -a "$D/classes.dex" | grep -iE 'provision' | sort -u | head -30

echo
echo "=== recline / 3dof / calibrat ==="
strings -a "$D/classes.dex" | grep -iE 'recline|3dof|6dof|calibrat|seethrough' | sort -u | head -30

echo
echo "=== property names it reads (pvr.* / persist.*) ==="
strings -a "$D/classes.dex" | grep -E '^(pvr|persist|ro|sys)\.[a-z0-9_.]+$' | sort -u | head -40

echo
echo "=== settings keys it might use ==="
strings -a "$D/classes.dex" | grep -iE '^[a-z_]*(setup|guide|first|provis|complete|finish)[a-z_]*$' | sort -u | head -30
echo DONE

#!/bin/bash
# The getter at 0x5b418 returns the cached tracking state (global 0x2049b0 +0x570).
# Which exported function is that, and which one SETS the field?
set -u
L=/mnt/f/PN2Lineage/notes/vrshell_lib/libPvr_UnitySDK.so
LOG=/mnt/f/PN2Lineage/notes/302_tracking_syms.txt
exec >"$LOG" 2>&1

echo "=== symbols covering / near 0x5b418 ==="
readelf -sW "$L" 2>/dev/null | awk '$2 ~ /^0000/ {
  addr=strtonum("0x" $2); size=$3+0;
  if (addr <= 0x5b418 && 0x5b418 < addr+size) printf "  COVERS  %s  @%s size %s\n", $8, $2, $3;
}'
echo "--- nearest exported symbols below/above ---"
nm -D --defined-only "$L" 2>/dev/null | sort | awk '{a=strtonum("0x"$1); if (a<=0x5b418) last=$0; else if (!shown) {print "  before: " last; print "  after : " $0; shown=1}}'

echo
echo "=== all Pvr_*TrackingMode symbols ==="
nm -D --defined-only "$L" 2>/dev/null | grep -iE 'trackingmode|trackingstate|SetTracking|GetTracking' | sort

echo
echo "=== what the SDK says about tracking state values ==="
strings -a "$L" | grep -iE 'trackingstate|tracking state|supportedTracking|trackingMode =' | sort -u | head -20
echo DONE

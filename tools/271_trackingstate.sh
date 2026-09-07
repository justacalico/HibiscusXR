#!/bin/bash
# "VrApi: trackingstate = 0x0,0x0" is logged by the app's SDK while pvrservice is
# publishing perfectly good rotation. Find the code that emits it and what it reads.
set -u
L=/mnt/f/PN2Lineage/notes/vrshell_lib/libPvr_UnitySDK.so
LOG=/mnt/f/PN2Lineage/notes/271_trackingstate.txt
exec >"$LOG" 2>&1

echo "=== the format string ==="
strings -a "$L" | grep -n 'trackingstate' | head

echo
echo "=== related tracking strings nearby ==="
strings -a "$L" | grep -iE 'trackingstate|tracking state|GetTrackingState|SetTrackingMode|GetTrackingMode|trackingmode' | sort -u | head -20

echo
echo "=== exported symbols about tracking ==="
nm -D --defined-only "$L" 2>/dev/null | grep -iE 'tracking|sensorstate|Pvr_Get' | head -30

echo
echo "=== who provides tracking data - shared memory? ==="
strings -a "$L" | grep -iE 'sharemem|share_mem|shareMemory|CtrlShareMem|/dev/ashmem' | sort -u | head -20
echo DONE

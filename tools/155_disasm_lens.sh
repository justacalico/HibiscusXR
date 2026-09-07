#!/bin/bash
# Confirm the null-deref theory by reading the actual code.
#
# fault addr 0x10 = a load at offset 0x10 from a null base. The suspicion is that
# the Java upcall PvrClient.getLensInfo() -> com.pvr.pvrservice.LensParameters
# returned null and native read fields straight out of it.
#
# Also: where are com/pvr/pvrservice/{LensParameters,DisplayInfo} supposed to come
# from? If they are not on the app's classpath the AIDL reply cannot be
# unmarshalled and null is exactly what the Java side would hand back.
set -u
REF=/mnt/f/PN2Lineage/ref/alvr-pico-legacy
SO=$REF/app/src/main/jniLibs/armeabi-v7a/libPvr_UnitySDK.so
JAR=$REF/app/libs/pvr_classes.jar
LOG=/mnt/f/PN2Lineage/notes/155_disasm.txt
exec >"$LOG" 2>&1

echo "=== what pvr_classes.jar provides (compile-time stubs) ==="
unzip -l "$JAR" 2>/dev/null | grep -iE 'pvrservice|psmart|vractivity' | awk '{print "  "$4}' | sort | head -40
echo
echo "  -- does the jar carry the pvrservice parcelables? --"
if unzip -l "$JAR" 2>/dev/null | grep -qi 'com/pvr/pvrservice/LensParameters'; then
  echo "  YES - LensParameters is in the jar"
else
  echo "  NO - LensParameters is NOT in the jar (must come from the platform)"
fi

OBJDUMP=""
for c in llvm-objdump-14 llvm-objdump arm-linux-gnueabi-objdump objdump; do
  command -v $c >/dev/null 2>&1 && { OBJDUMP=$c; break; }
done
echo
echo "=== using $OBJDUMP ==="

dis() {  # dis <symbol-substring> <bytes>
  local sym="$1" len="$2"
  local line addr
  line=$(nm -D --defined-only "$SO" 2>/dev/null | grep -m1 -- "$sym")
  [ -z "$line" ] && { echo "  symbol $sym not found"; return; }
  addr=0x$(echo "$line" | awk '{print $1}')
  echo
  echo "----- $sym  @ $addr -----"
  $OBJDUMP -d --triple=armv7-none-linux-androideabi \
     --start-address=$((addr & ~1)) --stop-address=$(( (addr & ~1) + len )) \
     "$SO" 2>/dev/null | tail -n +7
}

dis "PvrClientJava11getLensInfo"    420
dis "PvrClientJava14getDisplayInfo" 420
dis "psmvr_UpdateLensAndDisplayInfoFromVRService" 200

echo
echo "=== any load at #16 / #0x10 in those regions is the smoking gun ==="
echo DONE

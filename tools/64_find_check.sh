#!/bin/bash
# Locate the fatal CHECK inside libsensorservice.so:
#   CHECK failed: (int32_t)src.sensorType >= (int32_t)SensorType::DEVICE_PRIVATE_BASE
# It lives in convertToSensorEvent(), which is where Pico's non-standard sensor
# types (57, 58, 126, 127) kill system_server. No source tree here, so the plan is
# to understand the branch well enough to neutralise it.
NDK=/mnt/f/Android/Sdk/ndk/23.1.7779620/toolchains/llvm/prebuilt/linux-x86_64/bin
OBJDUMP=$NDK/llvm-objdump
LIB=/mnt/f/PN2Lineage/sensorpatch/libsensorservice.so
OUT=/mnt/f/PN2Lineage/notes/64_check.txt
exec >"$OUT" 2>&1

ls -l "$LIB"
echo
if [ ! -x "$OBJDUMP" ]; then
  echo "no linux llvm-objdump at $OBJDUMP; falling back to what's on PATH"
  OBJDUMP=$(command -v llvm-objdump || command -v objdump)
fi
echo "objdump: $OBJDUMP"
echo

echo "=== the assert string and who references it ==="
strings -t x "$LIB" | grep -i 'DEVICE_PRIVATE_BASE'
echo

echo "=== symbol for convertToSensorEvent ==="
"$OBJDUMP" -T "$LIB" 2>/dev/null | grep -i 'convertToSensorEvent'
nm -C "$LIB" 2>/dev/null | grep -i 'convertToSensorEvent'
echo

echo "=== disassembly of convertToSensorEvent ==="
SYM=$("$OBJDUMP" -d "$LIB" 2>/dev/null | grep -n 'convertToSensorEvent' | head -3)
echo "$SYM"
echo "--------"
"$OBJDUMP" -d "$LIB" 2>/dev/null \
  | awk '/<[^>]*convertToSensorEvent[^>]*>:/{f=1} f{print} f&&/^$/{c++; if(c>0 && NR>0 && $0==""){exit}}' \
  | head -160
echo DONE

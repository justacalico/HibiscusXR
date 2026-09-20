#!/usr/bin/env bash
# Patch CVService's native libs so the controller stack runs without
# pvrservice and without the SLPI sensor providers.
#
# libPvr_UnitySDKCV.so
#   0x787d6  mov r3,r0 -> movs r3,#2   force iServiceLoadOrder=2 so
#                                     InitServiceClient builds the local
#                                     stub client instead of the pvrservice
#                                     BitTube client, which crashes here
#   0x78996  bl InitServiceClient -> movs r0,#0; nop   GetDisplayInfo takes
#                                     its error path and keeps defaults
#
# libCVController.so
#   0x3b800  blx r0 -> nop            dead call through g_IMUsetEnable
#   0x3ee36  ldrb r7,[r7] -> movs r7,#0   per-packet sensor read always
#                                     takes the built-in access_eeprom path;
#                                     the g_EP* provider pointers stay null
#   0x3f200  ldrb.w -> mov.w r8,#1    skip spawning emSLPIThreadMain, which
#                                     calls g_setEnable unguarded
#   0x3f856  blx r3 -> nop            ~SpiSensor g_EPsetEnable(0)
#   0x3f860  blx r3 -> nop            ~SpiSensor g_IMUsetEnable(0)
#   0x92a69  01 -> 00                 shouldread_rx init byte, belt and
#                                     suspenders with the 0x3ee36 patch
#
# Usage: patch_cvservice_libs.sh [libdir]
#   libdir defaults to pvr_applibs/CVService/lib/arm, the dir 144_stage_full.sh
#   copies into fullstage. Run once after re-pulling stock libs.
set -euo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
DIR="${1:-$PN2_ROOT/pvr_applibs/CVService/lib/arm}"

patch() { # file offset hex-expected hex-new
  local f="$DIR/$1" off=$2 want=$3 new=$4
  local cur
  cur=$(od -A n -t x1 -N $(((${#new} + 1) / 3)) -j "$off" "$f" | tr -s ' ' | sed 's/^ //;s/ $//')
  if [ "$cur" = "$new" ]; then
    echo "  $1 @$off already patched"
    return
  fi
  if [ "$cur" != "$want" ]; then
    echo "FAIL $1 @$off: expected '$want', found '$cur'" >&2
    exit 1
  fi
  printf "$(echo "$new" | tr -d ' ' | sed 's/\([0-9a-f][0-9a-f]\)/\\x\1/g')" \
    | dd of="$f" bs=1 seek="$off" conv=notrunc status=none
  echo "  $1 @$off patched"
}

patch libPvr_UnitySDKCV.so 493526 "03 46" "02 23"
patch libPvr_UnitySDKCV.so 493974 "ff f7 0b ff" "00 20 00 bf"
patch libCVController.so 243712 "98 47" "00 bf"
patch libCVController.so 257590 "1f 78" "00 27"
patch libCVController.so 258560 "97 f8 00 80" "f0 4f 08 01"
patch libCVController.so 260182 "98 47" "00 bf"
patch libCVController.so 260192 "98 47" "00 bf"
patch libCVController.so 600745 "01" "00"

echo "done"

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
#   0x3d1f0  blx r3 -> nop            enableRX calls provider obj fn0, null
#                                     without pvrservice; SIGSEGV on every
#                                     ACTION_ENABLE_RX broadcast
#   0x3ee36  ldrb r7,[r7] -> movs r7,#0   per-packet sensor read always
#                                     takes the built-in access_eeprom path;
#                                     the g_EP* provider pointers stay null
#   0x3ee54  blx r3 -> nop            RX provider call in emSpiSensorThreadMain;
#                                     unreachable once 0x3ee36 is applied,
#                                     nop'd anyway for the unpatched case
#   0x3f200  ldrb.w -> mov.w r8,#1    skip spawning emSLPIThreadMain, which
#                                     calls g_setEnable unguarded
#   0x3f856  blx r3 -> nop            ~SpiSensor g_EPsetEnable(0)
#   0x3f860  blx r3 -> nop            ~SpiSensor g_IMUsetEnable(0)
#   0x92a69  01 -> 00                 shouldread_rx init byte, belt and
#                                     suspenders with the 0x3ee36 patch
#
# With pvrservice actually running (neo2-gated rc), two more nulls surface in
# RemoteService: a callee in the service-call chain returns with r4=0, which
# kills callers at their next GOT/canary load.
#
# libPvr_UnitySDKCV.so
#   0x4ba3e  bl UpdateCustomInfoFromVRService -> nop nop   skip the service
#                                     config override fetch inside
#                                     psmvr_InitConfigs; defaults from
#                                     LoadVariableFile stand
#
# libCVController.so
#   0x3dcca  ldr r3,[r4,r3] -> movs r3,#0    and
#   0x3dccc  ldrb r3,[r3] -> movs r3,#0      emSpiSensorThreadMain's run-flag
#                                     read faults on the zeroed r4; forcing
#                                     the flag to 0 takes the clean
#                                     pthread_detach/exit path. Harmless -
#                                     the SPI source it would read
#                                     (spidev2.0) does not exist in this
#                                     kernel anyway (notes/375)
#
# Usage: patch_cvservice_libs.sh [libdir]
#   libdir defaults to pvr_applibs/CVService/lib/arm, the dir 144_stage_full.sh
#   copies into fullstage. Run once after re-pulling stock libs.
set -euo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/../lib/env.sh"
DIR="${1:-$HIBISCUS_ROOT/pvr_applibs/CVService/lib/arm}"

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
patch libCVController.so 250352 "98 47" "00 bf"
patch libCVController.so 257590 "1f 78" "00 27"
patch libCVController.so 257620 "98 47" "00 bf"
patch libCVController.so 258560 "97 f8 00 80" "f0 4f 08 01"
patch libCVController.so 260182 "98 47" "00 bf"
patch libCVController.so 260192 "98 47" "00 bf"
patch libCVController.so 600745 "01" "00"
patch libPvr_UnitySDKCV.so 309822 "ff f7 d7 f8" "00 bf 00 bf"
patch libCVController.so 253642 "e3 58" "00 23"
patch libCVController.so 253644 "1b 78" "00 23"

echo "done"

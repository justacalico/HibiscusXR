#!/usr/bin/env bash
# Flash system-pn2-full.img and verify it boots.
#
# Two hard-won constraints, both non-optional:
#   * `fastboot oem pico unlock` is required once per fastboot session, and it
#     wedges if the session has been sitting idle - hence the timeout.
#   * `-S 128M` is required. The bootloader advertises max-download-size
#     536870912, but 512M chunks kill the USB link partway through
#     ("Write to device failed (no link)") and leave system half-written.
#
# Usage: flash_full.sh [image]
#   PN2_SERIAL  device serial if more than one is connected
set -euo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
IMG="${1:-$PN2_ROOT/out/system-pn2-full.img}"
LOG="${LOG:-$PN2_ROOT/notes/flash_$(date +%Y%m%d_%H%M%S).log}"
ADB=(adb ${PN2_SERIAL:+-s "$PN2_SERIAL"})
FB=(fastboot ${PN2_SERIAL:+-s "$PN2_SERIAL"})

log() { local l="[$(date +%H:%M:%S)] $*"; echo "$l"; mkdir -p "$(dirname "$LOG")"; echo "$l" >> "$LOG"; }

command -v adb >/dev/null || { echo "adb not in PATH" >&2; exit 1; }
command -v fastboot >/dev/null || { echo "fastboot not in PATH" >&2; exit 1; }
[ -f "$IMG" ] || { echo "image not found: $IMG" >&2; exit 1; }

log "image: $(stat -c%s "$IMG") bytes"

log "rebooting to bootloader"
"${ADB[@]}" reboot bootloader >/dev/null 2>&1 || true
sleep 10
for i in $(seq 20); do
  if fastboot devices | grep -q .; then break; fi
  [ "$i" -lt 20 ] || { log "ABORT: never reached fastboot"; exit 1; }
  sleep 2
done
# the fastboot serial is not the adb serial on this device - take whatever shows up
FB_SERIAL=$(fastboot devices | awk 'NR==1{print $1}')
FB=(fastboot ${FB_SERIAL:+-s "$FB_SERIAL"})
log "fastboot: $(fastboot devices | tr '\n' ' ')"

# unlock can hang on a stale session - cap it
if ! out=$(timeout 45 "${FB[@]}" oem pico unlock 2>&1); then
  log "ABORT: oem pico unlock hung or failed: $out"
  log "power-cycle into fastboot and re-run"
  exit 1
fi
log "unlock: $(echo "$out" | tr '\n' ' ')"

log "flashing system ($(du -h "$IMG" | cut -f1) in 128M chunks - this takes a few minutes)"
t0=$SECONDS
"${FB[@]}" -S 128M flash system "$IMG" 2>&1 | grep -iE 'sending|writing|okay|failed|error' | tee -a "$LOG" || true
log "flash finished in $((SECONDS-t0))s"

log "rebooting"
"${FB[@]}" reboot >/dev/null 2>&1 || true
log "done"

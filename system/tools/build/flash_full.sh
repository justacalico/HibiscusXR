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
#   HIBISCUS_SERIAL  device serial if more than one is connected
set -euo pipefail

HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
IMG="${1:-$HIBISCUS_ROOT/out/system-pn2-full.img}"
LOG="${LOG:-$HIBISCUS_ROOT/notes/flash_$(date +%Y%m%d_%H%M%S).log}"
ADB=(adb ${HIBISCUS_SERIAL:+-s "$HIBISCUS_SERIAL"})
FB=(fastboot ${HIBISCUS_SERIAL:+-s "$HIBISCUS_SERIAL"})

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
# the fastboot serial is not the adb serial on this device, so HIBISCUS_SERIAL
# often will not match the fastboot listing - but never guess when more than
# one device is present
mapfile -t fbdevs < <(fastboot devices | awk '{print $1}')
if [ -n "${HIBISCUS_SERIAL:-}" ]; then
  FB_SERIAL=""
  for d in "${fbdevs[@]}"; do [ "$d" = "$HIBISCUS_SERIAL" ] && FB_SERIAL=$d; done
  if [ -z "$FB_SERIAL" ]; then
    [ "${#fbdevs[@]}" -eq 1 ] || { log "ABORT: ${#fbdevs[@]} fastboot devices, none is $HIBISCUS_SERIAL"; exit 1; }
    FB_SERIAL=${fbdevs[0]}
  fi
else
  [ "${#fbdevs[@]}" -eq 1 ] || { log "ABORT: ${#fbdevs[@]} fastboot devices, set HIBISCUS_SERIAL"; exit 1; }
  FB_SERIAL=${fbdevs[0]}
fi
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
if out=$("${FB[@]}" -S 128M flash system "$IMG" 2>&1); then
  echo "$out" | grep -iE 'sending|writing|okay|failed|error' | tee -a "$LOG" || true
else
  rc=$?
  echo "$out" | tee -a "$LOG" || true
  log "ABORT: flash failed (rc=$rc) - system may be half-written, reflash before rebooting"
  exit 1
fi
log "flash finished in $((SECONDS-t0))s"

log "rebooting"
"${FB[@]}" reboot >/dev/null 2>&1 || true
log "done"

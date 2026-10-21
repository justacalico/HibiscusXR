#!/usr/bin/env bash
# Update an installed headset to a new release without wiping data.
#
# Flashes only the system partition - /data, installed apps and settings
# survive. The full-image script (tools/build/flash_full.sh) is the
# first-install path; this is the update path.
#
#   unxz system-hibiscus-full-<device>.img.xz
#   flash-update.sh system-hibiscus-full-<device>.img
#
#   HIBISCUS_SERIAL  fastboot device serial when more than one is connected
set -euo pipefail

IMG="${1:?usage: $0 <system.img>}"
[ -f "$IMG" ] || { echo "image not found: $IMG" >&2; exit 1; }
command -v fastboot >/dev/null || { echo "fastboot not in PATH" >&2; exit 1; }

mapfile -t devs < <(fastboot devices | awk '{print $1}')
[ "${#devs[@]}" -gt 0 ] || {
  echo "no fastboot device - 'adb reboot bootloader' first" >&2; exit 1; }
if [ -n "${HIBISCUS_SERIAL:-}" ]; then
  SERIAL=""
  for d in "${devs[@]}"; do [ "$d" = "$HIBISCUS_SERIAL" ] && SERIAL=$d; done
  if [ -z "$SERIAL" ]; then
    [ "${#devs[@]}" -eq 1 ] || { echo "${#devs[@]} devices, none is $HIBISCUS_SERIAL" >&2; exit 1; }
    SERIAL=${devs[0]}
  fi
else
  [ "${#devs[@]}" -eq 1 ] || { echo "${#devs[@]} fastboot devices, set HIBISCUS_SERIAL" >&2; exit 1; }
  SERIAL=${devs[0]}
fi
FB=(fastboot -s "$SERIAL")

# oem unlock command is required once per fastboot session on these
# bootloaders and can wedge on a stale session - cap it
if ! out=$(timeout 45 "${FB[@]}" oem pico unlock 2>&1); then
  echo "unlock hung or failed: $out" >&2
  echo "power-cycle into fastboot and re-run" >&2
  exit 1
fi

# -S 128M is not optional: larger sparse chunks kill the USB link mid-flash.
# No -w, no other partitions: userdata, apps and settings are preserved.
if ! "${FB[@]}" -S 128M flash system "$IMG"; then
  echo "flash failed - system may be half-written, reflash before rebooting" >&2
  exit 1
fi
"${FB[@]}" reboot
echo "updated - system flashed, userdata untouched"

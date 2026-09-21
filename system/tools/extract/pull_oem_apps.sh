#!/usr/bin/env bash
# Pull the /oem Pico apps off a stock device.
#
# /oem is NOT in the OTA package - a device is the only source. The pulled apps
# are deodexed like everything else, so they need the same
# unquicken -> inject -> re-sign treatment afterwards.
#
# Usage: pull_oem_apps.sh [output-dir] [app ...]
#   PN2_SERIAL  device serial if more than one is connected
set -uo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
OUT="${1:-$PN2_ROOT/oem_apps}"
shift 2>/dev/null || true

# launcher and home first; the media players are large and not on the path to VR
APPS=("$@")
[ "${#APPS[@]}" -gt 0 ] || APPS=(PVRLauncher PVRHome store2d provision2d ToBToolService)

ADB=(adb ${PN2_SERIAL:+-s "$PN2_SERIAL"})
command -v adb >/dev/null || { echo "adb not in PATH" >&2; exit 1; }
adb devices | awk '$2=="device"' | grep -q . || { echo "no device connected" >&2; exit 1; }

mkdir -p "$OUT"
for a in "${APPS[@]}"; do
  dst="$OUT/$a"
  mkdir -p "$dst"
  "${ADB[@]}" pull "/oem/priv-app/$a" "$OUT/" >/dev/null 2>&1
  apk=$(find "$dst" -name '*.apk' -printf '%s' 2>/dev/null | head -1)
  vdex=$(find "$dst" -name '*.vdex' -printf '%s' 2>/dev/null | head -1)
  so=$(find "$dst" -name '*.so' 2>/dev/null | wc -l)
  printf '%-16s apk=%-9s vdex=%-9s libs=%s\n' "$a" "${apk:-MISSING}" "${vdex:-MISSING}" "$so"
done
echo "total pulled: $(du -sm "$OUT" | cut -f1) MB -> $OUT"

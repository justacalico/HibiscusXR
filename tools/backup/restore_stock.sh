#!/usr/bin/env bash
# Restore a Pico Neo 2 to stock from a local partition backup.
#
# Use this if a GSI or custom system fails to boot. Restores only what is needed
# to get back to a working stock system; it deliberately DOES NOT touch the
# bootloader chain (xbl/abl/tz/hyp/...) because writing an older bootloader than
# the anti-rollback fuse value is the one true hard-brick vector on sdm845.
#
#   restore_stock.sh            # restore boot/dtbo/vbmeta/recovery + stock system
#   restore_stock.sh --all      # also restore vendor, oem, persist
#   restore_stock.sh --dry-run  # print the commands, flash nothing
#
#   PN2_SERIAL  fastboot/adb device serial if more than one is connected
set -euo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
BK="$PN2_ROOT/backup_nonEye"
OTA="$PN2_ROOT/images/ota_4.1.3"
ALL=0; DRY=0
for a in "$@"; do
  case "$a" in
    --all) ALL=1 ;;
    --dry-run) DRY=1 ;;
    *) echo "usage: $0 [--all] [--dry-run]" >&2; exit 2 ;;
  esac
done

# the fastboot serial is not the adb serial on this device - take whatever shows up
FB=fastboot

# system came from the OTA (stock, unmodified) - the on-device dd of it failed,
# which is fine because the OTA copy is byte-identical stock content.
declare -a plan=(
  "boot:$BK/boot.img"
  "dtbo:$BK/dtbo.img"
  "vbmeta:$BK/vbmeta.img"
  "recovery:$BK/recovery.img"
  "system:$OTA/system.img"
)
if [ "$ALL" -eq 1 ]; then
  plan+=(
    "vendor:$BK/vendor.img"
    "oem:$OTA/oem.bin"
    "persist:$BK/persist.img"
  )
fi

echo "=== restore plan ==="
missing=0
for e in "${plan[@]}"; do
  part="${e%%:*}"; f="${e#*:}"
  if [ -f "$f" ]; then s="present"; else s="MISSING"; missing=1; fi
  printf '%-10s %-60s %s\n' "$part" "$f" "$s"
done
echo
[ "$missing" -eq 0 ] || { echo "ABORT: missing images above. Not flashing a partial restore."; exit 1; }

run() { echo "+ $*"; [ "$DRY" -eq 0 ] && "$@"; return 0; }

if [ "$DRY" -eq 0 ]; then
  command -v fastboot >/dev/null || { echo "fastboot not in PATH" >&2; exit 1; }
  fastboot devices | grep -q . || { echo "no fastboot device" >&2; exit 1; }
  fbs=$(fastboot devices | awk 'NR==1{print $1}')
  FB=(fastboot ${fbs:+-s "$fbs"})
fi

# `fastboot oem pico unlock` is required once per fastboot session, and it can
# wedge on a stale session - hence the timeout.
run timeout 45 "${FB[@]}" oem pico unlock || { echo "ABORT: unlock hung or failed. Power-cycle into fastboot and retry."; exit 1; }

# -S 128M is not optional: 512M chunks kill the USB link partway through.
for e in "${plan[@]}"; do
  part="${e%%:*}"; f="${e#*:}"
  run "${FB[@]}" -S 128M flash "$part" "$f"
done

run "${FB[@]}" reboot
echo "done"

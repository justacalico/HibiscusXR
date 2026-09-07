#!/usr/bin/env bash
# Full partition backup of a Pico Neo 2 over adb.
# dd -> /sdcard -> adb pull -> delete on device, one partition at a time so
# device storage never holds more than one image.
#
# Usage: backup_partitions.sh [output-dir]
#   PN2_SERIAL  restrict to this device serial (recommended if >1 device)
#   PN2_ROOT    project root for the default output dir and notes/
set -uo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
DST="${1:-$PN2_ROOT/backup_nonEye}"
LOG="${LOG:-$PN2_ROOT/notes/backup_$(date +%Y%m%d_%H%M%S).log}"
ADB=(adb ${PN2_SERIAL:+-s "$PN2_SERIAL"})

log() { local l="[$(date +%H:%M:%S)] $*"; echo "$l"; mkdir -p "$(dirname "$LOG")"; echo "$l" >> "$LOG"; }

command -v adb >/dev/null || { echo "adb not in PATH" >&2; exit 1; }
mkdir -p "$DST"

# never back up (or touch) the wrong unit
mapfile -t devs < <(adb devices | awk '$2=="device"{print $1}')
[ "${#devs[@]}" -eq 1 ] || { log "ABORT: expected 1 device, found ${#devs[@]}"; exit 1; }
if [ -n "${PN2_SERIAL:-}" ] && [ "${devs[0]}" != "$PN2_SERIAL" ]; then
  log "ABORT: wrong device ${devs[0]}"; exit 1
fi
log "device OK: ${devs[0]}"

# toybox dd on 8.1 has no size suffixes; bytes only. Check root once up front.
if ! "${ADB[@]}" shell "su -c 'id -u'" 2>/dev/null | grep -q '^0'; then
  log "ABORT: su not available on device"; exit 1
fi

# ordered most-irreplaceable first, so a mid-run disconnect still saves what matters
parts=(
  persist picocfg
  modemst1 modemst2 fsg fsc
  boot dtbo vbmeta recovery
  misc frp keystore ssd devinfo sec cdt ddr
  xbl xbl_config abl aop tz hyp keymaster cmnlib cmnlib64
  devcfg qupfw storsec ImageFv dip apdp msadp limits spunvm
  sti toolsfv logfs splash mdtp mdtpsecapp
  bluetooth dsp modem
  vendor oem system
)

"${ADB[@]}" shell "su -c 'mkdir -p /sdcard/bk'" >/dev/null 2>&1

ok=0; fail=0
for p in "${parts[@]}"; do
  out="$DST/$p.img"
  if [ -f "$out" ]; then log "skip $p (already have it)"; ok=$((ok+1)); continue; fi
  if ! "${ADB[@]}" shell "su -c 'test -e /dev/block/bootdevice/by-name/$p'" 2>/dev/null; then
    log "skip $p (no such partition)"; continue
  fi

  r=$("${ADB[@]}" shell "su -c 'dd if=/dev/block/bootdevice/by-name/$p of=/sdcard/bk/$p.img bs=1048576 2>&1; echo RC=\$?'" 2>&1 | tr -d '\r' | tr '\n' ' ')
  if [[ "$r" != *RC=0* ]]; then log "FAIL dd $p : $r"; fail=$((fail+1)); continue; fi

  "${ADB[@]}" pull "/sdcard/bk/$p.img" "$out" >/dev/null 2>&1
  "${ADB[@]}" shell "su -c 'rm -f /sdcard/bk/$p.img'" >/dev/null 2>&1

  if [ -f "$out" ]; then
    log "$(printf 'OK   %-14s %10s MB' "$p" "$(awk "BEGIN{printf %.2f, $(stat -c%s "$out")/1048576}")")"
    ok=$((ok+1))
  else
    log "FAIL pull $p"; fail=$((fail+1))
  fi
done

"${ADB[@]}" shell "su -c 'rmdir /sdcard/bk'" >/dev/null 2>&1
tot=$(du -sh "$DST" | cut -f1)
log "DONE  ok=$ok fail=$fail  total=$tot  -> $DST"

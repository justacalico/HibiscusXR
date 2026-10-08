#!/usr/bin/env bash
# Turn the raw ext4 outputs into sparse images (what fastboot actually wants)
# and lay out the release directory.
set -euo pipefail
R="${PN2_ROOT:?}"
D="${1:-dist-out}"
DEVICE="${DEVICE:-neo2}"
SELF="$(cd "$(dirname "$0")/.." && pwd)"
. "$SELF/devices.env"
mkdir -p "$D"

# tools still write out/system-<codename>*.img; devices.env maps each raw
# name to its published device-suffixed stem (system-hibiscus*-$DEVICE).
for pair in $(device_outputs "$DEVICE"); do
  src=${pair%%:*}; pub=${pair##*:}
  [ -f "$R/out/$src.img" ] || { echo "missing $R/out/$src.img" >&2; exit 1; }
  img2simg "$R/out/$src.img" "$D/$pub.img"
  # GitHub release assets cap at 2G - the full sparse image is ~2.5G, so both
  # ship xz'd (the usual GSI convention: unxz, then fastboot flash)
  xz -1 -T0 "$D/$pub.img"
  echo "$pub.img.xz: $(stat -c%s "$R/out/$src.img") raw -> $(stat -c%s "$D/$pub.img.xz") sparse+xz"
done

tar -cJf "$D/build-logs.tar.xz" --exclude='*.so' -C "$R" notes

{
  echo "Hibiscus image build"
  echo "device:  $DEVICE"
  echo "version: ${HIBISCUS_VERSION:-dev}"
  echo "date:    $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "commit:  ${GITHUB_SHA:-local}"
  echo "run:     ${GITHUB_SERVER_URL:-}/${GITHUB_REPOSITORY:-}/${GITHUB_RUN_ID:-}"
  echo
  . "$SELF/manifest.env"
  echo "input pins:"
  set | grep -E '^PIN_' | sort | sed 's/^/  /'
  echo "source refs:"
  # locally no *_REF vars are set - grep exits 1 under set -e without this
  set | grep -E '^[A-Z]+_REF=' | sort | sed 's/^/  /' || true
  echo
  (cd "$D" && sha256sum *.img.xz)
} > "$D/build-manifest.txt"

(cd "$D" && find . -type f ! -name SHA256SUMS.txt -exec sha256sum {} + | sort -k2 > SHA256SUMS.txt)
ls -l "$D"

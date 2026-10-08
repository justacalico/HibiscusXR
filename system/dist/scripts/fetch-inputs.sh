#!/usr/bin/env bash
# Download the pinned input packages from this project's generic package
# registry and lay them out under $PN2_ROOT the way the tools/ scripts expect.
#
# What gets fetched depends on DEVICE: the GSI is shared, the rest is the
# union of the device's drivers' image.inputs (driver.json -> provider.py).
# DEVICE=vmd pulls gsi + blobs only - no Pico stack to download.
#
# Auth: the inputs-reader deploy token (GL_DT_USER / GL_DT_TOKEN secrets).
# Everything here is proprietary Pico material or derived from it - the repos
# deliberately don't carry it, which is why it travels as packages.
set -euo pipefail

R="${PN2_ROOT:?}"
PID="${GL_PROJECT_ID:?}"
BASE="https://gitlab.com/api/v4/projects/$PID/packages/generic"
SELF="$(cd "$(dirname "$0")/.." && pwd)"
. "$SELF/manifest.env"
. "$SELF/devices.env"

DEVICE="${DEVICE:-neo2}"
device_known "$DEVICE" || { echo "unknown DEVICE '$DEVICE'" >&2; exit 1; }
export HSVR_DRIVERS="${HSVR_DRIVERS:-$(device_drivers "$DEVICE")}"
PROV="$SELF/../../tools/provider.py"

# pin name -> registry package / file
pkg_of() {
  case "$1" in
    GSI)        echo "gsi" ;;
    STACK)      echo "pvr-stack" ;;
    APPS)       echo "pvr-apps" ;;
    APPLIBS)    echo "pvr-applibs" ;;
    OEM)        echo "oem-final" ;;
    LOADINGRES) echo "loadingres" ;;
    BLOBS)      echo "blobs" ;;
    SEETHROUGH) echo "seethrough" ;;
  esac
}
file_of() {
  case "$1" in
    GSI)        echo "lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img.xz" ;;
    STACK)      echo "pvr_stack.tar.xz" ;;
    APPS)       echo "pvr_apps_final.tar.xz" ;;
    APPLIBS)    echo "pvr_applibs.tar.xz" ;;
    OEM)        echo "oem_final.tar.xz" ;;
    LOADINGRES) echo "LoadingRes.tar.xz" ;;
    BLOBS)      echo "blobs.tar.xz" ;;
    SEETHROUGH) echo "seethrough.tar.xz" ;;
  esac
}
pin_of() { eval "echo \$PIN_$1"; }

W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT

dl() { # dl <pkg> <ver> <file>
  echo ">> $1/$2/$3"
  curl -fsSL --retry 3 -u "$GL_DT_USER:$GL_DT_TOKEN" \
      -o "$W/$3" "$BASE/$1/$2/$3"
  stat -c '   %s bytes' "$W/$3"
}

mkdir -p "$R/gsi" "$R/notes" "$R/out" "$R/.stub"

# GSI is the shared base every device starts from; the driver's own
# image.inputs list the rest.
PINS="GSI $(python3 "$PROV" inputs | tr '\n' ' ')"
echo "device: $DEVICE (drivers: $HSVR_DRIVERS) -> inputs: $PINS"
for p in $PINS; do
  dl "$(pkg_of "$p")" "$(pin_of "$p")" "$(file_of "$p")"
done

for p in $PINS; do
  case "$p" in
    GSI)
      mv "$W/$(file_of GSI)" "$R/gsi/" ;;
    LOADINGRES)
      # LoadingRes feeds a fake stock image for 144_stage_full.sh
      mkdir -p "$R/.stub/media"
      tar -xJf "$W/LoadingRes.tar.xz" -C "$R/.stub/media" ;;
    *)
      tar -xJf "$W/$(file_of "$p")" -C "$R" ;;
  esac
done

echo "=== inputs laid out ($DEVICE) ==="
# informational only: du exits nonzero when a device legitimately lacks a dir
# (vmd has no pvr_*), and this is the last command so it would fail the step
du -sh "$R"/{pvr_stack,pvr_apps_final,pvr_applibs,oem_final,seethrough,overlay_pvr,airsvc,rfsa,qvr,cdsp,fan,linklibs,build,notes,gsi} 2>/dev/null || true

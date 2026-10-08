#!/usr/bin/env bash
# The whole pipeline, end to end. Assumes fetch-inputs.sh already populated
# $PN2_ROOT and the source repos (tools, overlay, shim, vrhome) are cloned.
#
# The numbered tools/ scripts write their logs into notes/ and mostly don't
# exit nonzero on failure - each step greps the log for its success marker.
set -euo pipefail
R="${PN2_ROOT:?}"
T="$R/tools/build"
N="$R/notes"
SELF="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$N" "$R/out"

step() { echo; echo "######## $* ########"; }
fail() { echo "FAILED: $*" >&2; exit 1; }

# The image is built for one device, published as *-$DEVICE.img.xz. The
# device matrix in devices.env picks which hsvr drivers the provider
# activates - which image steps run and which inputs are fetched. The
# monado runtime itself always compiles every drivers/*/monado driver,
# so the neo2 image still answers the vmd pose channel under qemu.
DEVICE="${DEVICE:-neo2}"
. "$SELF/devices.env"
device_known "$DEVICE" || fail "unknown DEVICE '$DEVICE'"
export HSVR_DRIVERS="${HSVR_DRIVERS:-$(device_drivers "$DEVICE")}"
step "device: $DEVICE (drivers: $HSVR_DRIVERS)"

# Every input the chain below reads, in one shot - the steps fail one at a
# time otherwise, which costs a rebuild cycle per missing dir. The source
# dirs come from layout-sources.sh --list, the same list the workflow lays
# out - preflight catches a local build root missing one of them.
step "preflight: required inputs"
miss=0
need() { [ -e "$R/$1" ] || { echo "  MISSING $1"; miss=$((miss+1)); }; }
while read -r p; do
  need "$p"
done < <("$SELF/scripts/layout-sources.sh" --list)
# shared: every device is the same GSI base + overlay + shell; blobs carry
# build/keys (apk re-sign) and linklibs (shim link targets)
for p in gsi overlay/media/bootanimation/desc.txt build/keys linklibs overlay/lib64; do
  need "$p"
done
[ -f "$R/gsi/gsi_raw.img" ] || \
  [ -f "$R/gsi/lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img.xz" ] || \
  { echo "  MISSING gsi (no gsi_raw.img, no xz to make it)"; miss=$((miss+1)); }
case "$DEVICE" in
  neo2)
    for p in \
      .stub/media/LoadingRes \
      pvr_stack pvr_apps_final pvr_applibs oem_final \
      overlay_pvr airsvc rfsa qvr cdsp fan seethrough \
      notes/libart-patched.so \
      notes/vrshell_lib/libPvr_UnitySDK.patched2.so; do
      need "$p"
    done
    # the stock gles blob: qlibs/linklibs copy, or a device on adb to pull from
    [ -f "$R/linklibs/libGLESv2_adreno.so" ] || \
      [ -f "$R/notes/qlibs/libGLESv2_adreno.so" ] || \
      adb devices 2>/dev/null | grep -q "device$" || \
      { echo "  MISSING libGLESv2_adreno.so (linklibs, notes/qlibs, no adb device)"; miss=$((miss+1)); }
    ;;
esac
[ "$miss" -eq 0 ] || fail "$miss inputs missing - see system/dist/README.md (running locally)"
echo "  all inputs present"
ran() { # ran <logfile> <marker>
  local log="$N/$1" mark="$2"
  grep -qiE "error|failed|missing|not found" "$log" 2>/dev/null && {
    echo "--- $log (errors) ---"
    grep -niE "error|failed|missing|not found" "$log" | tail -30
  }
  echo "--- $log (tail) ---"
  tail -25 "$log" 2>/dev/null || true
  grep -q "$mark" "$log" || fail "marker '$mark' not in $log"
}

step "gsi: unxz + simg2img"
cd "$R/gsi"
if [ ! -f gsi_raw.img ]; then
  xz -dk lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img.xz
  simg2img lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img gsi_raw.img
  rm -f lineage-17.1-20210808-UNOFFICIAL-treble_arm64_avS.img
fi
ls -l gsi_raw.img

step "shims from source"
"$SELF/scripts/build-shims.sh"

step "adb props into gsi_raw"
"$SELF/scripts/patch-gsi-props.sh"

step "version stamp into gsi_raw"
"$SELF/scripts/write-version.sh"

step "clean image: overlay into GSI (143)"
bash "$T/143_build_image2.sh" || true
ran 143_build.txt "BUILD OK"

step "strip stock apps (410)"
bash "$T/410_strip_stock_apps.sh" || true
ran 410_strip.txt "STRIP OK"

if [ "$DEVICE" = "neo2" ]; then
  step "LoadingRes stub image"
  "$SELF/scripts/make-loadingres-img.sh"
fi

# device payloads come from the driver provider - each
# drivers/<name>/driver.json lists its own steps, the pipeline only runs
# them in order and checks their markers
PROV="$SELF/../../tools/provider.py"
run_phase() { # run_phase <stage|inject>; leaves the count in PHASE_RAN
  local phase="$1" run log marker
  PHASE_RAN=0
  while IFS=$'\t' read -r run log marker; do
    [ -n "$run" ] || continue
    PHASE_RAN=$((PHASE_RAN + 1))
    step "driver $phase: $(basename "$run")"
    bash "$run" || true
    [ -n "$marker" ] && ran "$log" "$marker"
  done < <(python3 "$PROV" steps "$phase")
}

step "drivers: stage payloads"
run_phase stage
if [ "$PHASE_RAN" -gt 0 ]; then
  echo "  staged: $(find "$R/fullstage" -type f | wc -l) files, $(du -sh "$R/fullstage" | cut -f1)"
  [ "$DEVICE" != "neo2" ] || [ -d "$R/fullstage/media/LoadingRes" ] || fail "LoadingRes not staged"
fi

if [ "$DEVICE" = "neo2" ]; then
  step "grow + inject stack (145)"
  bash "$T/145_build_full.sh" || true
  ran 145_full.txt "fits"

  step "overlay fixes + pvr payload + shell stack (267)"
  bash "$T/267_build_full.sh" || true
  ran 267_build.txt "BUILD OK"
fi

step "drivers: inject payloads"
run_phase inject

step "verify"
if [ "$DEVICE" = "neo2" ]; then
  bash "$T/268_verify_img.sh" || true
  tail -30 "$N/268_verify.txt"
fi

step "final assertions"
for pair in $(device_outputs "$DEVICE"); do
  src=${pair%%:*}
  IMG="$R/out/$src.img"
  [ -f "$IMG" ] || fail "$src.img missing - the device payload never produced it"
  e2fsck -fn "$IMG" >/dev/null 2>&1 || fail "$src.img fsck dirty"
  echo "  $src.img: $(stat -c%s "$IMG") bytes, fsck clean"
done
if [ "$DEVICE" = "neo2" ]; then
  sz=$(stat -c%s "$R/out/system-pn2-full.img")
  [ "$sz" -le 3943694336 ] || fail "full image $sz > partition 3943694336"
fi

step "BUILD PIPELINE OK"

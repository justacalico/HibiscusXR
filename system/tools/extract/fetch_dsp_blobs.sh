#!/bin/bash
HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
# Populate the gitignored DSP/tracking blobs from the stock dump so the display
# fix (300_img_dsp.sh + 301_mdsp_img.sh) can be re-run from a fresh checkout.
#
# None of these are ours - they are Pico/Qualcomm firmware pulled from the
# owner's own stock images, never committed (see each repo's README).
#
#   rfsa/*.so            <- system.img  /lib/rfsa/adsp/   (DSP-side skels + SLAM)
#   cdsp/vendor/*.so     <- vendor.img  /lib/             (the *_system SONAME is
#                          rejected on version-need - the /vendor copies are the
#                          ones the qvr cdsp stub actually links)
#   cdsp/vendor/libqvr_cdsp_driver_stub.so <- system.img /lib/
#   cdsp/lib{,64}/*_system.so            <- system.img /lib{,64}/
set -u
SYSIMG=${HIBISCUS_ROOT}/images/ota_4.1.3/system.img
VENIMG=${HIBISCUS_ROOT}/images/vendor.img
LOG=${HIBISCUS_ROOT}/notes/fetch_dsp_blobs.txt
exec >"$LOG" 2>&1

fail=0
[ -f "$SYSIMG" ] || { echo "missing $SYSIMG"; exit 1; }
[ -f "$VENIMG" ] || { echo "missing $VENIMG"; exit 1; }

# rdump <img> <imgpath> <destdir> - copy a whole directory out of a stock image
rdump() {
  mkdir -p "$3"
  debugfs -R "rdump $2 $3" "$1" 2>/dev/null
}
# pull <img> <imgpath> <destfile> - copy a single file out of a stock image
pull() {
  mkdir -p "$(dirname "$3")"
  debugfs -R "dump $2 $3" "$1" 2>/dev/null
  [ -s "$3" ] || { echo "  FAIL $2"; fail=$((fail+1)); }
}

echo "=== rfsa skels (system.img /lib/rfsa/adsp) ==="
rm -rf "$HIBISCUS_ROOT/rfsa/adsp_tmp"; mkdir -p "$HIBISCUS_ROOT/rfsa/adsp_tmp"
rdump "$SYSIMG" /lib/rfsa/adsp "$HIBISCUS_ROOT/rfsa/adsp_tmp"
# rdump lands files under adsp_tmp/adsp/ - move the .so up into rfsa/
find "$HIBISCUS_ROOT/rfsa/adsp_tmp" -name '*.so' -exec mv -t "$HIBISCUS_ROOT/rfsa/" {} +
rm -rf "$HIBISCUS_ROOT/rfsa/adsp_tmp"
n=$(ls "$HIBISCUS_ROOT"/rfsa/*.so 2>/dev/null | wc -l)
echo "  rfsa: $n skels"
[ "$n" -ge 15 ] || { echo "  FAIL rfsa: expected >=15 got $n"; fail=$((fail+1)); }

echo "=== cdsp/vendor (vendor.img /lib) ==="
pull "$VENIMG" /lib/libcdsprpc.so "$HIBISCUS_ROOT/cdsp/vendor/libcdsprpc.so"
pull "$VENIMG" /lib/libmdsprpc.so "$HIBISCUS_ROOT/cdsp/vendor/libmdsprpc.so"

echo "=== cdsp/vendor stub (system.img /lib) ==="
pull "$SYSIMG" /lib/libqvr_cdsp_driver_stub.so "$HIBISCUS_ROOT/cdsp/vendor/libqvr_cdsp_driver_stub.so"

echo "=== cdsp _system variants (system.img /lib + /lib64) ==="
for a in lib lib64; do
  for n in libadsprpc_system libcdsprpc_system libsdsprpc_system; do
    pull "$SYSIMG" /$a/$n.so "$HIBISCUS_ROOT/cdsp/$a/$n.so"
  done
done

echo
echo "=== result ==="
for d in "$HIBISCUS_ROOT/rfsa" "$HIBISCUS_ROOT/cdsp/vendor" "$HIBISCUS_ROOT/cdsp/lib" "$HIBISCUS_ROOT/cdsp/lib64"; do
  echo "  $(ls "$d"/*.so 2>/dev/null | wc -l) .so in ${d#$HIBISCUS_ROOT/}"
done
[ "$fail" -eq 0 ] && echo "DSP BLOBS OK" || echo "HAD $fail FAILURES"

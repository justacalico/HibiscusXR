#!/bin/bash
. "$(dirname "${BASH_SOURCE[0]}")/../lib/env.sh"
# Fold the overlay into a flashable system.img.  v2.
#
# v1 reported "ok" for every write and then verified nothing, because it parsed
# `debugfs stat` output that never matched. Verify with `ls -l` on the parent
# directory instead, and make put() fail loudly.
#
# Ships NO proprietary Pico content: GSI + our own work only (two sensorservice
# patches, the ABI shim, the init rc files, VINTF override, build.prop). Pico's
# blobs are installed afterwards by the end user from their own device.
#
# Pass "recopy" to start from a fresh copy of the GSI; otherwise the existing
# output image is updated in place (the 2GB copy is the slow part).
set -u
GSI=${HIBISCUS_ROOT}/gsi/gsi_raw.img
OV=${HIBISCUS_ROOT}/overlay
OUT=${HIBISCUS_ROOT}/out
IMG=$OUT/system-base.img
LOG=${HIBISCUS_ROOT}/notes/143_build.txt
MODE=${1:-update}
exec >"$LOG" 2>&1

mkdir -p "$OUT"
if [ "$MODE" = "recopy" ] || [ ! -f "$IMG" ]; then
  echo "=== copying fresh from GSI ==="
  cp -f "$GSI" "$IMG"
fi
ls -l "$IMG"

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo
echo "=== patched libs ==="
put "$OV/lib64/libsensorservice.so" "/lib64/libsensorservice.so" 644
put "$OV/lib64/libshim_pvr.so"      "/lib64/libshim_pvr.so"      644

echo
echo
echo "=== boot animation ==="
# overlay/media/bootanimation is the unpacked zip. The panel is 3840x2160
# split between the lenses, so frames carry one logo per eye centre - a single
# centred phone-style animation lands on the seam and each eye sees half a
# logo. bootanimation wants desc.txt first and every entry stored, not
# deflated; python's zipfile so we don't depend on zip(1) behaviour.
BA="$OUT/bootanimation.zip"
python3 - "$OV/media/bootanimation" "$BA" <<'EOF'
import os, sys, zipfile
src, out = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(out, "w", zipfile.ZIP_STORED) as z:
    z.write(os.path.join(src, "desc.txt"), "desc.txt")
    for part in sorted(d for d in os.listdir(src) if d.startswith("part")):
        for f in sorted(os.listdir(os.path.join(src, part))):
            z.write(os.path.join(src, part, f), f"{part}/{f}")
EOF
mkd /media
put "$BA" "/media/bootanimation.zip" 644
# The stock lookup order is apex -> product -> oem -> system, and the GSI
# ships its phone-sized animation under product/apex - drop whichever of
# those exists so ours is the one that plays.
for d in /product/media /system/product/media; do
  if debugfs -R "stat $d" "$IMG" 2>/dev/null | grep -q "Inode:"; then
    put "$BA" "$d/bootanimation.zip" 644
    debugfs -w -R "rm $d/bootanimation-dark.zip" "$IMG" >/dev/null 2>&1
  fi
done
for v in /apex/com.android.bootanimation/etc/bootanimation.zip \
         /apex/com.android.bootanimation.apex; do
  if debugfs -R "stat $v" "$IMG" 2>/dev/null | grep -q "Inode:"; then
    debugfs -w -R "rm $v" "$IMG" >/dev/null 2>&1
    echo "  removed $v"
  fi
done

echo
echo "=== build.prop ==="
TMP=$(mktemp -d)
debugfs -R "dump /build.prop $TMP/build.prop" "$IMG" 2>/dev/null
if [ -s "$TMP/build.prop" ]; then
  if grep -q 'ro.product.manufacturer=Hibiscus' "$TMP/build.prop"; then
    echo "  already patched ($(stat -c%s "$TMP/build.prop") bytes)"
  else
    # guarantee a trailing newline first. Without this the append glues onto the
    # last existing line - that produced
    #   ro.sf.hwrotation=90ro.product.device=PICOA7B10
    # in both shipped images, silently dropping ro.product.device.
    [ -n "$(tail -c1 "$TMP/build.prop")" ] && echo "" >> "$TMP/build.prop"
    grep -q '^ro.product.manufacturer=' "$TMP/build.prop" || echo 'ro.product.manufacturer=Hibiscus' >> "$TMP/build.prop"
    put "$TMP/build.prop" "/build.prop" 600
  fi
  echo "  --- our block as it now reads ---"
  grep -E '^ro\.(build\.product|product\.(device|model))=' "$TMP/build.prop" | sed 's/^/    /'
else
  echo "  ERROR: could not read build.prop"
  fail=$((fail+1))
fi

echo
echo "=== hashes: image vs overlay (must match) ==="
for pair in "/lib64/libshim_pvr.so $OV/lib64/libshim_pvr.so" \
            "/lib64/libsensorservice.so $OV/lib64/libsensorservice.so"; do
  set -- $pair
  debugfs -R "dump $1 $TMP/x" "$IMG" 2>/dev/null
  a=$(md5sum "$TMP/x" 2>/dev/null | cut -d' ' -f1)
  b=$(md5sum "$2" | cut -d' ' -f1)
  if [ "$a" = "$b" ]; then printf '  OK    %s\n' "$1"; else printf '  FAIL  %s (%s vs %s)\n' "$1" "$a" "$b"; fail=$((fail+1)); fi
done

echo
echo "=== repair pass ==="
# debugfs write/rm cycles leave the free block and inode accounting
# inconsistent - the pristine GSI fscks clean, an image we have written to does
# not. Repair, then verify the repair actually took. Shipping an image that
# needs a repair on first mount is not acceptable. One pass can surface a
# second problem (an orphaned dir lands in lost+found on pass one, its '..'
# fixup only happens on the next), so keep repairing until clean, bounded.
for i in 1 2 3; do
  e2fsck -fy "$IMG" 2>&1 | tail -6
  e2fsck -fn "$IMG" >/dev/null 2>&1 && break
done

echo
echo "=== fsck after repair (must be clean) ==="
if e2fsck -fn "$IMG" > "$TMP/fsck.txt" 2>&1; then
  tail -2 "$TMP/fsck.txt"
  echo "  filesystem CLEAN"
else
  tail -8 "$TMP/fsck.txt"
  echo "  filesystem STILL DIRTY"
  fail=$((fail+1))
fi

echo
ls -l "$IMG"
echo
if [ "$fail" -eq 0 ]; then echo "BUILD OK"; else echo "BUILD HAD $fail FAILURES"; fi
echo DONE

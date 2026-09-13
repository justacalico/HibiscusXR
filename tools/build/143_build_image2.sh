#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
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
GSI=${PN2_ROOT}/gsi/gsi_raw.img
OV=${PN2_ROOT}/overlay
OUT=${PN2_ROOT}/out
IMG=$OUT/system-pn2.img
LOG=${PN2_ROOT}/notes/143_build.txt
MODE=${1:-update}
exec >"$LOG" 2>&1

mkdir -p "$OUT"
if [ "$MODE" = "recopy" ] || [ ! -f "$IMG" ]; then
  echo "=== copying fresh from GSI ==="
  cp -f "$GSI" "$IMG"
fi
ls -l "$IMG"

fail=0
put() {   # put <local> <img-path> <mode>
  local src="$1" dst="$2" mode="$3"
  local dir base
  dir=$(dirname "$dst"); base=$(basename "$dst")
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  # verify for real: is it listed, and is the size right
  local want got
  want=$(stat -c%s "$src")
  # debugfs `ls -l` columns: inode mode (type) uid gid SIZE date time name
  # so size is field 6. $(NF-4) was landing on gid and always read 0.
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then
    printf '  OK    %-42s %s bytes\n' "$dst" "$got"
  else
    printf '  FAIL  %-42s wrote %s, image reports "%s"\n' "$dst" "$want" "${got:-absent}"
    fail=$((fail+1))
  fi
}

echo
echo "=== init scripts ==="
for f in pn2-vintf.rc pn2-snd.rc pn2-settings.rc pn2-power.rc pvrservice.rc; do
  [ -f "$OV/etc/init/$f" ] && put "$OV/etc/init/$f" "/etc/init/$f" 644
done

echo
echo "=== VINTF manifest override ==="
debugfs -w -R "mkdir /etc/pn2" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /etc/pn2 mode 040755" "$IMG" >/dev/null 2>&1
put "$OV/etc/pn2/vendor_manifest.xml" "/etc/pn2/vendor_manifest.xml" 644

echo
echo "=== patched libs ==="
put "$OV/lib64/libsensorservice.so" "/lib64/libsensorservice.so" 644
put "$OV/lib64/libshim_pvr.so"      "/lib64/libshim_pvr.so"      644

echo
echo "=== headset buttons: key layouts ==="
debugfs -w -R "mkdir /usr/keylayout" "$IMG" >/dev/null 2>&1
for f in gpio-keys.kl dc_detect.kl; do
  put "$OV/usr/keylayout/$f" "/usr/keylayout/$f" 644
done

echo
echo "=== headset buttons: libinput keycode labels ==="
# the GSI's libinput does not know Pico's labels, so gpio-keys.kl would be
# discarded whole and Confirm would arrive as ENTER through Generic.kl.
# lib2dToVr only clicks on keycodes 1001/1002/96, so the ok button does
# nothing inside PVR Home. Patch the image's own libinput in place: dump,
# repurpose unused TV_* table entries, write back.
TMP=$(mktemp -d)
for lib in lib64 lib; do
  debugfs -R "dump /$lib/libinput.so $TMP/libinput.so" "$IMG" 2>/dev/null
  if [ -s "$TMP/libinput.so" ] && python3 "$PN2_ROOT/tools/patch/401_patch_libinput.py" \
      "$TMP/libinput.so" "$TMP/libinput.patched.so" >/dev/null 2>&1; then
    put "$TMP/libinput.patched.so" "/$lib/libinput.so" 644
  else
    echo "  SKIP    /$lib/libinput.so (patch failed or missing)"
    fail=$((fail+1))
  fi
  rm -f "$TMP/libinput.so" "$TMP/libinput.patched.so"
done

echo
echo "=== build.prop ==="
TMP=$(mktemp -d)
debugfs -R "dump /build.prop $TMP/build.prop" "$IMG" 2>/dev/null
if [ -s "$TMP/build.prop" ]; then
  if grep -q 'Pico Neo 2 (A7B10) port additions' "$TMP/build.prop"; then
    echo "  already patched ($(stat -c%s "$TMP/build.prop") bytes)"
  else
    # guarantee a trailing newline first. Without this the append glues onto the
    # last existing line - that produced
    #   ro.sf.hwrotation=90ro.product.device=PICOA7B10
    # in both shipped images, silently dropping ro.product.device.
    [ -n "$(tail -c1 "$TMP/build.prop")" ] && echo "" >> "$TMP/build.prop"
    cat "$OV/props.append" >> "$TMP/build.prop"
    [ -n "$(tail -c1 "$TMP/build.prop")" ] && echo "" >> "$TMP/build.prop"
    sed -i 's/^ro\.build\.product=.*/ro.build.product=PICOA7B10/' "$TMP/build.prop"
    grep -q '^ro.product.device=' "$TMP/build.prop" || echo 'ro.product.device=PICOA7B10' >> "$TMP/build.prop"
    grep -q '^ro.product.model='  "$TMP/build.prop" || echo 'ro.product.model=Pico Neo 2' >> "$TMP/build.prop"
    put "$TMP/build.prop" "/build.prop" 600
  fi
  echo "  --- our block as it now reads ---"
  grep -A6 'Pico Neo 2 (A7B10) port additions' "$TMP/build.prop" | head -8 | sed 's/^/    /'
  grep -E '^ro\.(build\.product|product\.(device|model))=' "$TMP/build.prop" | sed 's/^/    /'
else
  echo "  ERROR: could not read build.prop"
  fail=$((fail+1))
fi

echo
echo "=== hashes: image vs overlay (must match) ==="
for pair in "/lib64/libshim_pvr.so $OV/lib64/libshim_pvr.so" \
            "/lib64/libsensorservice.so $OV/lib64/libsensorservice.so" \
            "/usr/keylayout/gpio-keys.kl $OV/usr/keylayout/gpio-keys.kl" \
            "/usr/keylayout/dc_detect.kl $OV/usr/keylayout/dc_detect.kl" \
            "/etc/pn2/vendor_manifest.xml $OV/etc/pn2/vendor_manifest.xml"; do
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
# needs a repair on first mount is not acceptable.
e2fsck -fy "$IMG" 2>&1 | tail -6

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

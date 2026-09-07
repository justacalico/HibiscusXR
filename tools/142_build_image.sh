#!/bin/bash
# Fold the overlay into a flashable system.img.
#
# Deliberately ships NO proprietary Pico content. The image is the GSI plus our
# own work: the two sensorservice patches, the ABI shim, four init rc files, the
# VINTF manifest override and the build.prop additions. Pico's blobs are added
# afterwards by the end user from their own device - see PROPRIETARY-PVR.md and
# the companion extract script.
#
# debugfs, not a loop mount: no root needed, and the host cannot be affected.
# The image root IS /system, so paths inside are /etc/... not /system/etc/...
set -u
GSI=/mnt/f/PN2Lineage/gsi/gsi_raw.img
OV=/mnt/f/PN2Lineage/overlay
OUT=/mnt/f/PN2Lineage/out
IMG=$OUT/system-pn2.img
LOG=/mnt/f/PN2Lineage/notes/142_build.txt
exec >"$LOG" 2>&1

mkdir -p "$OUT"
echo "=== source GSI ==="
ls -l "$GSI"

echo
echo "=== copying (this is the slow part) ==="
cp -f "$GSI" "$IMG"
ls -l "$IMG"

# --- helper: write a local file into the image, then fix mode/owner ----------
put() {   # put <local> <img-path> <mode>
  local src="$1" dst="$2" mode="$3"
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  if debugfs -w -R "write $src $dst" "$IMG" 2>&1 | grep -qi 'error\|not found'; then
    echo "  FAILED  $dst"
    return 1
  fi
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0"  "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0"  "$IMG" >/dev/null 2>&1
  printf '  ok      %-42s %s bytes\n' "$dst" "$(stat -c%s "$src")"
}

echo
echo "=== init scripts ==="
for f in pn2-vintf.rc pn2-snd.rc pn2-settings.rc pn2-power.rc; do
  put "$OV/etc/init/$f" "/etc/init/$f" 644
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
echo "=== build.prop additions ==="
TMP=$(mktemp -d)
debugfs -R "dump /build.prop $TMP/build.prop" "$IMG" 2>/dev/null
if [ -s "$TMP/build.prop" ]; then
  echo "  original: $(stat -c%s "$TMP/build.prop") bytes"
  if grep -q 'Pico Neo 2 (A7B10) port additions' "$TMP/build.prop"; then
    echo "  already contains our block, skipping"
  else
    cat "$OV/props.append" >> "$TMP/build.prop"
    # device identity: pvrservice and friends key off these
    sed -i 's/^ro\.build\.product=.*/ro.build.product=PICOA7B10/' "$TMP/build.prop"
    grep -q '^ro.product.device=' "$TMP/build.prop" || echo 'ro.product.device=PICOA7B10' >> "$TMP/build.prop"
    grep -q '^ro.product.model='  "$TMP/build.prop" || echo 'ro.product.model=Pico Neo 2' >> "$TMP/build.prop"
    put "$TMP/build.prop" "/build.prop" 600
    echo "  patched : $(stat -c%s "$TMP/build.prop") bytes"
  fi
else
  echo "  ERROR: could not read build.prop out of the image"
fi

echo
echo "=== verify what landed ==="
for p in /etc/init/pn2-vintf.rc /etc/init/pn2-snd.rc /etc/init/pn2-settings.rc \
         /etc/init/pn2-power.rc /etc/pn2/vendor_manifest.xml \
         /lib64/libshim_pvr.so /lib64/libsensorservice.so; do
  line=$(debugfs -R "stat $p" "$IMG" 2>/dev/null | grep -E '^Size:' | head -1)
  printf '  %-42s %s\n' "$p" "${line:-MISSING}"
done

echo
echo "=== fsck ==="
e2fsck -fn "$IMG" 2>&1 | tail -8

echo
echo "=== result ==="
ls -l "$IMG"
echo DONE

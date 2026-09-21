#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Build vendor.img from the OTA and pull the key identity files out of system/vendor/oem.
set -u
B=${PN2_ROOT}
IMG=$B/images
OTA=$IMG/ota_4.1.3
EX=$B/extracted
LOG=$B/notes/04_recon.log
exec >"$LOG" 2>&1

mkdir -p "$EX/props"

echo "=== toolchain ==="
for t in brotli dtc simg2img unzip 7z cpio debugfs readelf; do
  printf '%-12s' "$t"
  if command -v "$t" >/dev/null 2>&1; then echo OK; else echo MISSING; fi
done
echo

echo "=== decompress vendor.new.dat.br ==="
if [ ! -f "$OTA/vendor.new.dat" ]; then
  brotli -d -o "$OTA/vendor.new.dat" "$OTA/vendor.new.dat.br"
  echo "brotli exit $?"
fi
ls -l "$OTA/vendor.new.dat"
echo

echo "=== rebuild vendor.img ==="
if [ ! -f "$IMG/vendor.img" ]; then
  python3 "$B/tools/03_sdat2img.py" "$OTA/vendor.transfer.list" "$OTA/vendor.new.dat" "$IMG/vendor.img"
fi
ls -l "$IMG/vendor.img"
file "$IMG/vendor.img"
echo

echo "=== image identification ==="
for i in "$IMG/snapshot_lun0/system.bin" "$IMG/vendor.img" "$OTA/oem.bin"; do
  printf '%-46s ' "$i"
  file -b "$i" 2>/dev/null | head -1
done
echo

echo "=== root listing: system.bin ==="
debugfs -R "ls -l /" "$IMG/snapshot_lun0/system.bin" 2>/dev/null
echo
echo "=== root listing: vendor.img ==="
debugfs -R "ls -l /" "$IMG/vendor.img" 2>/dev/null
echo
echo "=== root listing: oem.bin ==="
debugfs -R "ls -l /" "$OTA/oem.bin" 2>/dev/null
echo

echo "=== pulling build.prop files ==="
pull() {  # image  path-in-image  outname
  if debugfs -R "dump -p $2 $EX/props/$3" "$1" >/dev/null 2>&1 && [ -s "$EX/props/$3" ]; then
    echo "OK   $3  ($(wc -l < "$EX/props/$3") lines)"
  else
    echo "MISS $1 : $2"
    rm -f "$EX/props/$3"
  fi
}
pull "$IMG/snapshot_lun0/system.bin" /build.prop            system-build.prop
pull "$IMG/snapshot_lun0/system.bin" /system/build.prop     system-build.prop.alt
pull "$IMG/snapshot_lun0/system.bin" /default.prop          system-default.prop
pull "$IMG/vendor.img"               /build.prop            vendor-build.prop
pull "$IMG/vendor.img"               /default.prop          vendor-default.prop
pull "$OTA/oem.bin"                  /build.prop            oem-build.prop
echo

echo "=== vintf manifests ==="
pull "$IMG/vendor.img" /etc/vintf/manifest.xml            vendor-manifest.xml
pull "$IMG/vendor.img" /manifest.xml                      vendor-manifest-root.xml
pull "$IMG/vendor.img" /etc/vintf/compatibility_matrix.xml vendor-matrix.xml
pull "$IMG/snapshot_lun0/system.bin" /etc/vintf/manifest.xml system-manifest.xml
pull "$IMG/snapshot_lun0/system.bin" /compatibility_matrix.xml system-matrix.xml
echo

echo "RECON DONE"

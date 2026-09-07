#!/bin/bash
# Extract vendor + system/etc/init to WSL-native storage and inventory the VR stack.
set -u
B=/mnt/f/PN2Lineage
SYS=$B/images/snapshot_lun0/system.bin
VEN=$B/images/vendor.img
W=/home/justin/pn2
N=$B/notes

mkdir -p "$W" "$N"

# ---------- extract ----------
if [ ! -d "$W/vendor" ]; then
  mkdir -p "$W/vendor"
  debugfs -R "rdump / $W/vendor" "$VEN" > "$N/06_extract.log" 2>&1
fi
if [ ! -d "$W/sysinit" ]; then
  mkdir -p "$W/sysinit"
  debugfs -R "rdump /etc/init $W/sysinit" "$SYS" >> "$N/06_extract.log" 2>&1
  debugfs -R "rdump /etc/permissions $W/sysinit" "$SYS" >> "$N/06_extract.log" 2>&1
fi
V=$W/vendor
[ -d "$V/vendor" ] && V=$V/vendor   # rdump may nest

# ---------- A: tree ----------
{
echo "=== vendor top level ==="
ls -la "$V"
echo
echo "=== vendor dir sizes ==="
du -sh "$V"/* 2>/dev/null | sort -rh
echo
echo "=== vendor/bin/hw (HAL daemons) ==="
ls -la "$V/bin/hw" 2>/dev/null
echo
echo "=== vendor/lib64/hw ==="
ls -la "$V/lib64/hw" 2>/dev/null
echo
echo "=== vendor/firmware (count + VR/cam/eye related) ==="
ls "$V/firmware" 2>/dev/null | wc -l
ls "$V/firmware" 2>/dev/null | grep -iE 'vr|cam|eye|tobii|track|adsp|cdsp|slpi' | head -40
} > "$N/06a_vendor_tree.log" 2>&1

# ---------- B: VR-named files across vendor ----------
{
echo "=== vendor files matching VR/tracking/eye ==="
find "$V" -iregex '.*\(qvr\|pvr\|pxr\|pico\|tobii\|eye\|track\|sixdof\|6dof\|slam\|imu\|hmd\|vr\).*' \
  -printf '%10s  %p\n' 2>/dev/null | sed "s|$V|/vendor|" | sort -k2
} > "$N/06b_vendor_vrfiles.log" 2>&1

# ---------- C: init rc referencing VR ----------
{
echo "=== VENDOR init rc mentioning vr/qvr/pvr/track/eye ==="
for f in "$V"/etc/init/*.rc; do
  [ -f "$f" ] || continue
  if grep -qiE 'qvr|pvr|pxr|vr_|/vr|tobii|eye|track|sixdof' "$f"; then
    echo "########## $f ##########"
    cat "$f"
    echo
  fi
done
echo
echo "=== SYSTEM init rc mentioning vr ==="
for f in $(find /home/justin/pn2/sysinit -name '*.rc' 2>/dev/null); do
  if grep -qiE 'qvr|pvr|pxr|vr_|/vr|tobii|eye|track|sixdof' "$f"; then
    echo "########## $f ##########"
    cat "$f"
    echo
  fi
done
} > "$N/06c_init_rc.log" 2>&1

# ---------- D: vendor manifest + build.prop ----------
{
echo "=== vendor/build.prop ==="
cat "$V/build.prop" 2>/dev/null
echo
echo "=== vendor manifest.xml ==="
cat "$V/manifest.xml" 2>/dev/null || cat "$V/etc/vintf/manifest.xml" 2>/dev/null
} > "$N/06d_vendor_props.log" 2>&1

# ---------- E: QVR interface surface ----------
{
for lib in libqvrservice_client.so libqvrservice.so libpvrservice.so libqvrcamera_client.so; do
  for p in "$V/lib64/$lib" "$V/lib/$lib"; do
    [ -f "$p" ] || continue
    echo "################ $p ################"
    echo "--- file ---"; file -b "$p"
    echo "--- NEEDED ---"
    readelf -d "$p" 2>/dev/null | grep NEEDED | sed 's/^ *//'
    echo "--- exported functions (defined, global) ---"
    readelf --dyn-syms -W "$p" 2>/dev/null | awk '$4=="FUNC" && $5=="GLOBAL" && $7!="UND" {print $8}' | sort -u
    echo
  done
done
echo "=== same libs in /system (64+32) ==="
find /home/justin/pn2 -name 'libqvr*' -o -name 'libpvr*' 2>/dev/null | head
} > "$N/06e_qvr_iface.log" 2>&1

echo "VENDORHUNT DONE"
ls -la "$N"/06*.log

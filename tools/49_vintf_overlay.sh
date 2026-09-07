#!/system/bin/sh
# Move the boot-HAL fix off the vendor partition and into a system-side bind mount.
#   1. stage the patched manifest under /system/etc/pn2/
#   2. restore /vendor/manifest.xml to Pico's original bytes
#   3. install the early-init bind mount
# After this, /vendor is byte-identical to stock and the fix lives entirely in the
# image we actually ship.
set -e
B=/data/local/tmp/manifest.orig.xml     # exact bytes, taken before the first edit

[ -f "$B" ] || { echo "ABORT: no pristine backup at $B"; exit 1; }
echo "pristine backup: $(stat -c%s "$B") bytes, boot refs $(grep -c 'hardware\.boot' "$B")"
grep -q 'hardware\.boot' "$B" || { echo "ABORT: backup has no boot decl, wrong file"; exit 1; }

mount -o rw,remount /system
mkdir -p /system/etc/pn2

# patched copy = current (already-stripped) /vendor/manifest.xml
cp /vendor/manifest.xml /system/etc/pn2/vendor_manifest.xml
chmod 644 /system/etc/pn2/vendor_manifest.xml
chown root:root /system/etc/pn2/vendor_manifest.xml
echo "staged patched: $(stat -c%s /system/etc/pn2/vendor_manifest.xml) bytes, boot refs $(grep -c 'hardware\.boot' /system/etc/pn2/vendor_manifest.xml)"

# put the init rc in place
cp /data/local/tmp/pn2-vintf.rc /system/etc/init/pn2-vintf.rc
chmod 644 /system/etc/init/pn2-vintf.rc
chown root:root /system/etc/init/pn2-vintf.rc
sync
mount -o ro,remount /system

# now hand /vendor back its original file
mount -o rw,remount /vendor
cat "$B" > /vendor/manifest.xml
chmod 644 /vendor/manifest.xml
chown root:root /vendor/manifest.xml
sync
mount -o ro,remount /vendor

echo "--- final state ---"
echo "vendor  : $(stat -c%s /vendor/manifest.xml) bytes, boot refs $(grep -c 'hardware\.boot' /vendor/manifest.xml)  (want 22604 / 1)"
echo "overlay : $(stat -c%s /system/etc/pn2/vendor_manifest.xml) bytes, boot refs $(grep -c 'hardware\.boot' /system/etc/pn2/vendor_manifest.xml)  (want 22335 / 0)"
ls -l /system/etc/init/pn2-vintf.rc
echo DONE

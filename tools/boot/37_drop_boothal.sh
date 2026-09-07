#!/system/bin/sh
# Remove the android.hardware.boot HAL declaration from /vendor/manifest.xml.
#
# Pico declares android.hardware.boot@1.0::IBootControl but ships no
# implementation - no /vendor/bin/hw binary, no bootctrl.*.so, and the device is
# non-A/B (no slot_suffix, no _a/_b partitions). Stock 8.1 never called it so the
# lie was free. Android 10 vold calls IBootControl::getService() from
# cp_needsCheckpoint() on every boot; libhidl blocks forever on an interface that
# is declared-but-unregistered, which wedges vold's main mutex and watchdog-kills
# system_server every 80s.
#
# Deleting the declaration makes getService() return null immediately, which is
# the answer vold wants on a non-A/B device.

M=/vendor/manifest.xml
B=/data/local/tmp/manifest.orig.xml
N=/data/local/tmp/manifest.new.xml

cp "$M" "$B" || { echo "FAIL: backup"; exit 1; }
echo "backed up to $B ($(stat -c%s "$B") bytes)"

# Drop whole <hal>...</hal> blocks whose body mentions android.hardware.boot.
awk '
  /<hal / { inhal=1; buf=$0; hit=0; next }
  inhal {
    buf = buf "\n" $0
    if ($0 ~ /android\.hardware\.boot</) hit=1
    if ($0 ~ /<\/hal>/) { if (!hit) print buf; inhal=0 }
    next
  }
  { print }
' "$M" > "$N" || { echo "FAIL: awk"; exit 1; }

echo "orig $(stat -c%s "$M")  new $(stat -c%s "$N")"
echo "boot refs remaining: $(grep -c 'hardware\.boot' "$N")"
echo "hal open/close: $(grep -c '<hal ' "$N") / $(grep -c '</hal>' "$N")"

# refuse to commit anything that looks wrong
[ "$(grep -c 'hardware\.boot' "$N")" -eq 0 ] || { echo "ABORT: boot ref survived"; exit 1; }
[ "$(grep -c '<hal ' "$N")" -eq "$(grep -c '</hal>' "$N")" ] || { echo "ABORT: unbalanced"; exit 1; }
[ "$(stat -c%s "$N")" -gt 20000 ] || { echo "ABORT: new file too small"; exit 1; }
grep -q '</manifest>' "$N" || { echo "ABORT: no closing manifest tag"; exit 1; }

mount -o rw,remount /vendor || { echo "FAIL: remount rw"; exit 1; }
cat "$N" > "$M" || { echo "FAIL: write"; mount -o ro,remount /vendor; exit 1; }
chmod 644 "$M"
chown root:root "$M"
sync
mount -o ro,remount /vendor
echo "committed. now: $(stat -c%s "$M") bytes, boot refs $(grep -c 'hardware\.boot' "$M")"
echo DONE

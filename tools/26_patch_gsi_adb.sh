#!/bin/bash
# Force adb on at first boot in the GSI.
#
# With a freshly wiped /data there is no persisted USB config, so the gadget
# comes up as MTP only and we have no way in. init falls back to the build.prop
# default for persist.* props when /data has none, so setting it here works for
# exactly the case we care about.
#
# Verity is disabled on this device, so editing the system image is safe.
set -u
PW="$1"
D=/mnt/f/PN2Lineage/gsi
R=$D/gsi_raw.img
M=/mnt/gsi_edit
L=/mnt/f/PN2Lineage/notes/26_patch_gsi.log
exec >"$L" 2>&1

echo "$PW" | sudo -S mkdir -p "$M" 2>/dev/null
echo "$PW" | sudo -S mount -o loop,rw "$R" "$M" 2>/dev/null
if ! mountpoint -q "$M"; then echo "ABORT: mount failed"; exit 1; fi
echo "mounted $R at $M"
echo

echo "=== build.prop before ==="
grep -nE 'adb|usb|debuggable|secure' "$M/build.prop" || echo "(no adb/usb lines)"
echo

# Strip any existing versions of the keys we set, then append ours.
echo "$PW" | sudo -S sed -i -E '/^(persist\.sys\.usb\.config|ro\.adb\.secure|ro\.debuggable|ro\.secure)=/d' "$M/build.prop"
echo "$PW" | sudo -S tee -a "$M/build.prop" >/dev/null <<'EOF'

# --- added for Pico Neo 2 bring-up -------------------------------------------
# Fresh /data has no persisted USB config, so default the gadget to adb+mtp and
# drop adb auth. Bring-up only; remove for any release build.
persist.sys.usb.config=adb,mtp
ro.adb.secure=0
ro.debuggable=1
ro.secure=0
EOF

echo "=== build.prop after ==="
grep -nE 'persist.sys.usb.config|ro.adb.secure|ro.debuggable|ro.secure' "$M/build.prop"
echo

# Some GSIs also read prop.default; mirror it there if present.
if [ -f "$M/etc/prop.default" ]; then
  echo "=== also patching /etc/prop.default ==="
  echo "$PW" | sudo -S sed -i -E '/^(persist\.sys\.usb\.config|ro\.adb\.secure|ro\.debuggable|ro\.secure)=/d' "$M/etc/prop.default"
  echo "$PW" | sudo -S tee -a "$M/etc/prop.default" >/dev/null <<'EOF'
persist.sys.usb.config=adb,mtp
ro.adb.secure=0
ro.debuggable=1
ro.secure=0
EOF
  grep -nE 'persist.sys.usb.config|ro.adb.secure|ro.debuggable' "$M/etc/prop.default"
fi
echo

echo "$PW" | sudo -S umount "$M"
echo "unmounted; fsck:"
e2fsck -fy "$R" 2>&1 | tail -5
ls -l "$R"
echo DONE

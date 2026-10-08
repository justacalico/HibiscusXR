#!/usr/bin/env bash
# vmd image payload - runs as drivers/vmd/driver.json's inject step.
#
# Builds out/system-vmd.img: the shared clean base plus the shared shell
# stack (270), nothing Pico. What makes the image "vmd" is the guest side
# already inside the runtime - hsvr_drv_vmd answers the host TCP channel
# (:7781) and the rest is stock Android on virtio.
#
# There is no clean/full split for vmd and no device partition cap: qemu
# takes the raw image on virtio-blk as-is.
set -u
R="${PN2_ROOT:?}"
IMG="$R/out/system-vmd.img"
LOG="$R/notes/500_vmd.txt"
exec >"$LOG" 2>&1

fail=0

echo "=== starting from the clean image ==="
CLEAN="$R/out/system-pn2.img"
[ -f "$CLEAN" ] || { echo "FAIL: $CLEAN missing - 143 never ran"; exit 1; }
cp -f "$CLEAN" "$IMG"
ls -l "$IMG"

echo
echo "=== grow to 3000M (shell stack + headroom; no partition cap on vmd) ==="
# grow only - truncating a bigger source would silently eat the fs
if [ "$(stat -c%s "$IMG")" -lt 3145728000 ]; then
  truncate -s 3000M "$IMG"
fi
e2fsck -fy "$IMG" >/dev/null 2>&1
resize2fs "$IMG" 2>&1 | tail -3
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Free blocks|Block count'

echo
echo "=== drop the pvrservice hook (no Pico compositor exists here) ==="
debugfs -w -R "rm /etc/init/pvrservice.rc" "$IMG" >/dev/null 2>&1

echo
echo "=== wireless adb on by default ==="
# A VM has no usb gadget to enumerate and no way to tap through a setup
# flow - adb over the qemu hostfwd is the only way in. persist.* props
# seed from build.prop on a fresh /data, and the guest socket is only
# reachable through vmd's loopback forwards, so nothing on the LAN sees it.
T=$(mktemp -d)
debugfs -R "dump /build.prop $T/build.prop" "$IMG" >/dev/null 2>&1
grep -q '^persist\.pn2\.adbwifi=' "$T/build.prop" || \
  echo "persist.pn2.adbwifi=1" >> "$T/build.prop"
debugfs -w -R "rm /build.prop" "$IMG" >/dev/null 2>&1
debugfs -w -R "write $T/build.prop /build.prop" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop mode 0100600" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop uid 0" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop gid 0" "$IMG" >/dev/null 2>&1
rm -rf "$T"

echo
echo "=== shell stack (270 - identical to the neo2 userspace) ==="
bash "$R/tools/build/270_shell_stack.sh" "$IMG" \
    || { echo "FAIL 270_shell_stack"; fail=$((fail+1)); }

echo
echo "=== self-check ==="
for p in /app/PN2Panels/PN2Panels.apk /app/PN2Hud/PN2Hud.apk \
         /app/MonadoOpenXR/MonadoOpenXR.apk \
         /app/MonadoOpenXR/lib/arm64/libopenxr_monado.so \
         /etc/init/pn2-home.rc /etc/init/pn2-openxr.rc \
         /etc/hibiscus-release; do
  if debugfs -R "stat $p" "$IMG" 2>/dev/null | grep -q "Inode:"; then
    echo "  OK    $p"
  else
    echo "  MISSING $p"; fail=$((fail+1))
  fi
done

echo
if [ "$fail" -eq 0 ]; then
  echo "VMD IMG OK"
else
  echo "VMD IMG HAD $fail FAILURES"
  exit 1
fi

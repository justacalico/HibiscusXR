#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Fold this session's work into system-pn2-full.img.
#
# Only the pn2 hardware payload lives here - the shared userspace (shell,
# panel apps, keyboard, Monado runtime, platform re-sign, fsck) runs last
# via 270_shell_stack.sh, the same script vmd images go through.
#
# Everything below was proven on the live device first. Deliberately NOT included:
#   - the libPvr_UnitySDKExt11.so x28 patch: its code cave landed inside
#     PVR::BufferedFile::~BufferedFile and hung VRShell. The see-through app needs
#     that fix, but it needs a properly verified cave first.
#   - the CVService packages.xml ABI correction: that lives in /data, not /system.
#     On a fresh flash PackageManager scans with lib/arm already present and
#     derives armeabi-v7a by itself, so it does not need carrying.
#
# Verify every write by size - debugfs reports success even when it silently
# refuses to overwrite an existing file, which is why each put() re-reads it.
set -u
IMG=${PN2_ROOT}/out/system-pn2-full.img
LOG=${PN2_ROOT}/notes/267_build.txt
exec >"$LOG" 2>&1

fail=0
put() {   # put <local> <img-path> <mode>
  local src="$1" dst="$2" mode="$3"
  local dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  [ -f "$src" ] || { printf '  MISSING SOURCE %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then
    printf '  OK    %-52s %12s\n' "$dst" "$got"
  else
    printf '  FAIL  %-52s wrote %s, image says "%s"\n' "$dst" "$want" "${got:-absent}"
    fail=$((fail+1))
  fi
}
mkd() {
  debugfs -w -R "mkdir $1" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 mode 040755" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $1 gid 0" "$IMG" >/dev/null 2>&1
}

echo "=== free space before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Free blocks|Block size'
ls -l "$IMG"

OV=${PN2_ROOT}/overlay_pvr
AIR=${PN2_ROOT}/airsvc
SHIM=${PN2_ROOT}/shim
INIT=${PN2_ROOT}/overlay/etc/init
ST=${PN2_ROOT}/seethrough

# The shims are our own code - rebuild them from source when the .so is absent
# (fresh checkout), so the image is reproducible without a prebuilt binary.
if [ ! -s "$SHIM/libshim_pvr.so" ] || [ ! -s "$SHIM/libshim_air.so" ] || [ ! -s "$SHIM/libskia_stub.so" ]; then
  echo "=== shims missing - building from shim/ source ==="
  bash "$SHIM/build.sh" || echo "  shim build failed - put() will report the gap"
fi

echo
echo "=== linker whitelist (the big one: without this every Pico dlopen returns null) ==="
PLIB=$(mktemp)
cp "$OV/public.libraries.txt" "$PLIB"
# the blobs tarball can lag overlay_pvr; make sure the qvrservice client
# libs the OpenXR stack needs are whitelisted either way
for l in libqvrservice_client.so libdrm.so; do
  grep -qx "$l" "$PLIB" || echo "$l" >> "$PLIB"
done
put "$PLIB" /etc/public.libraries.txt 644
rm -f "$PLIB"

echo
echo "=== restored blobs, both ABIs ==="
for l in libvirtualinputclient libairclient libSafetyArea libImageGrid libdatabuffer; do
  put "$OV/lib64/$l.so" "/lib64/$l.so" 644
  put "$OV/lib/$l.so"   "/lib/$l.so"   644
done
put "$AIR/lib64/libvirtualinput.so" /lib64/libvirtualinput.so 644
put "$AIR/lib/libvirtualinput.so"   /lib/libvirtualinput.so   644

echo
echo "=== passthrough camera service + virtual input daemons ==="
put "$AIR/bin/airservice"    /bin/airservice    755
put "$AIR/bin/virtual_input" /bin/virtual_input 755

echo
echo "=== isolated 8.1 chain (only airservice sees these, via LD_LIBRARY_PATH) ==="
mkd /lib64/pvr_air
put "$AIR/lib64/libairservice.so" /lib64/pvr_air/libairservice.so 644
put "$AIR/lib64/libaircamera.so"  /lib64/pvr_air/libaircamera.so  644
put "$SHIM/libskia_stub.so"       /lib64/pvr_air/libskia.so       644
put "$SHIM/libshim_air.so"        /lib64/pvr_air/libshim_air.so   644

echo
echo "=== shims ==="
put "$SHIM/libshim_pvr.so" /lib64/libshim_pvr.so 644

echo
echo "=== headset buttons: key layouts + libinput keycode labels ==="
# Without gpio-keys.kl the gpio-keys device falls back to Generic.kl and the
# confirm button arrives as ENTER; lib2dToVr only treats 1001/1002/96 as
# confirm, so the ok button is dead inside PVR Home. libinput must also know
# Pico's keycode labels or the whole .kl is discarded at parse time.
mkd /usr/keylayout
put ${PN2_ROOT}/overlay/usr/keylayout/gpio-keys.kl /usr/keylayout/gpio-keys.kl 644
put ${PN2_ROOT}/overlay/usr/keylayout/dc_detect.kl /usr/keylayout/dc_detect.kl 644
TMPD=$(mktemp -d)
for lib in lib64 lib; do
  debugfs -R "dump /$lib/libinput.so $TMPD/libinput.so" "$IMG" 2>/dev/null
  if [ ! -s "$TMPD/libinput.so" ]; then
    echo "  FAIL    /$lib/libinput.so missing from image"; fail=$((fail+1))
  elif strings "$TMPD/libinput.so" | grep -q DEFINE_HOME; then
    echo "  OK    /$lib/libinput.so already patched"
  elif python3 "$PN2_ROOT/tools/patch/401_patch_libinput.py" \
      "$TMPD/libinput.so" "$TMPD/libinput.patched.so" >/dev/null 2>&1; then
    put "$TMPD/libinput.patched.so" "/$lib/libinput.so" 644
  else
    echo "  FAIL    /$lib/libinput.so patch failed"; fail=$((fail+1))
  fi
  rm -f "$TMPD/libinput.so" "$TMPD/libinput.patched.so"
done
rm -rf "$TMPD"

echo
echo "=== init scripts ==="
# the generic shell hooks (pn2-home, pn2-openxr, pn2-adbwifi) are in
# 270_shell_stack.sh - this list is only the pn2 hardware services
put "$INIT/pn2-airservice.rc" /etc/init/pn2-airservice.rc 644
put "$INIT/pn2-qvrd.rc"       /etc/init/pn2-qvrd.rc       644
put "$INIT/pn2-settings.rc"   /etc/init/pn2-settings.rc   644
put "$INIT/pn2-vulkan.rc"     /etc/init/pn2-vulkan.rc     644
put "$INIT/pn2-ipd.rc"        /etc/init/pn2-ipd.rc        644
put "$INIT/pn2-dof.rc"        /etc/init/pn2-dof.rc        644
put "$INIT/pn2-theme.rc"      /etc/init/pn2-theme.rc      644
put "$INIT/pn2-env.rc"        /etc/init/pn2-env.rc        644

echo
echo "=== settings bridge daemons ==="
# settings.Global -> persist props; the props are the channel every native
# renderer can read live without a settings-provider round trip
put ${PN2_ROOT}/overlay/bin/pn2-ipdd /bin/pn2-ipdd 755
put ${PN2_ROOT}/overlay/bin/pn2-dofd /bin/pn2-dofd 755
put ${PN2_ROOT}/overlay/bin/pn2-themed /bin/pn2-themed 755
put ${PN2_ROOT}/overlay/bin/pn2-envd /bin/pn2-envd 755

echo
echo "=== ART trampoline patch (mov sp,x28 -> mov sp,x29) ==="
put ${PN2_ROOT}/notes/libart-patched.so /apex/com.android.runtime.release/lib64/libart.so 644

echo
echo "=== VRShell x28 patch (recompute struct base from x27) ==="
put ${PN2_ROOT}/notes/vrshell_lib/libPvr_UnitySDK.patched2.so /priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so 644

echo
echo "=== see-through calibration app ==="
mkd /priv-app/seethroughsetting
mkd /priv-app/seethroughsetting/lib
mkd /priv-app/seethroughsetting/lib/arm64
put "$ST/seethroughsetting-signed.apk" /priv-app/seethroughsetting/seethroughsetting.apk 644
for f in "$ST"/lib/arm64/*.so; do
  put "$f" "/priv-app/seethroughsetting/lib/arm64/$(basename "$f")" 644
done

echo
echo "=== OpenXR stack: Turnip Vulkan driver (sdm845) ==="
# This replaces the stock VR path for raw OpenXR apps. Verified live:
#   - hwvulkan picks /vendor/lib64/hw/vulkan.sdm845.so first; pn2-vulkan.rc
#     bind-mounts this Turnip over the vendor path at post-fs so the Adreno
#     module never loads. Works with an untouched stock vendor.img.
#   - pvrservice is the broken compositor behind the black-display bug; its rc
#     is removed so nothing starts it. qvrd stays - it owns the tracking cams.
XR=${PN2_ROOT}/hsvr
# Always run the build - it is incremental, and skipping on a stale out/
# dir once shipped an unpatched Turnip in the image.
bash "$XR/turnip/build.sh" || echo "  turnip build failed - put() will report the gap"
mkd /lib64/hw
put "$XR/turnip/out/libvulkan_freedreno.so" /lib64/hw/vulkan.sdm845.so 644
# Turnip is an NDK build and DT_NEEDEDs libc++_shared.so. Every load under
# /vendor/lib64 lands in the sphal namespace, whose search paths only cover
# /odm and /vendor - so the bind-mounted driver could not resolve it even
# though the lib sits in /system/lib64. The default link is the allowlist
# for reaching across; adding the soname there lets sphal resolve it from
# the default namespace. Verified on device: WiVRn creates a session and
# presents frames once this line is in.
LD27=$(mktemp)
debugfs -R "dump /etc/ld.config.27.txt $LD27" "$IMG" >/dev/null 2>&1
if [ -s "$LD27" ]; then
  grep -q 'shared_libs.*libc++_shared' "$LD27" || \
    sed -i '/^namespace\.sphal\.link\.default\.shared_libs/a namespace.sphal.link.default.shared_libs += libc++_shared.so' "$LD27"
  put "$LD27" /etc/ld.config.27.txt 644
else
  echo "  FAIL  could not read /etc/ld.config.27.txt from image"; fail=$((fail+1))
fi
rm -f "$LD27"
debugfs -w -R "rm /etc/init/pvrservice.rc" "$IMG" >/dev/null 2>&1
echo "  removed /etc/init/pvrservice.rc"

echo
echo "=== shell stack (270 - shared with every device) ==="
# vrhome/HUD, the flutter panels, keyboard, Monado runtime, the platform
# re-sign and the fsck all run there so vmd builds the identical userspace.
bash "$PN2_ROOT/tools/build/270_shell_stack.sh" "$IMG" \
    || { echo "FAIL 270_shell_stack"; fail=$((fail+1)); }

echo
if [ "$fail" -eq 0 ]; then echo "BUILD OK"; else echo "BUILD HAD $fail FAILURES"; fi
echo DONE

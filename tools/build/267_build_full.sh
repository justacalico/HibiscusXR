#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Fold this session's work into system-pn2-full.img.
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
put "$OV/public.libraries.txt" /etc/public.libraries.txt 644

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
put "$INIT/pn2-airservice.rc" /etc/init/pn2-airservice.rc 644
put "$INIT/pn2-qvrd.rc"       /etc/init/pn2-qvrd.rc       644
put "$INIT/pn2-adbwifi.rc"    /etc/init/pn2-adbwifi.rc    644
put "$INIT/pn2-settings.rc"   /etc/init/pn2-settings.rc   644
put "$INIT/pn2-home.rc"       /etc/init/pn2-home.rc       644

echo
echo "=== ART trampoline patch (mov sp,x28 -> mov sp,x29) ==="
put ${PN2_ROOT}/notes/libart-patched.so /apex/com.android.runtime.release/lib64/libart.so 644

echo
echo "=== VRShell x28 patch (recompute struct base from x27) ==="
put ${PN2_ROOT}/notes/vrshell_lib/libPvr_UnitySDK.patched2.so /priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so 644

echo
echo "=== shell split: env (gitlab.neosalsa.home) + HUD (gitlab.neosalsa.hud) ==="
# The env is a platform-signed NativeActivity owning display 0. The HUD is a
# platform-signed service holding a TYPE_SYSTEM_OVERLAY window; it owns the
# virtual displays, so panels live over the env and over summoned VR apps.
# HOME role, hidden API whitelist and disabling the stock Pico homes are
# first-boot work in pn2-home.rc - the role holder lives in /data and cannot
# be baked in.
#
# Always rebuild from source and verify the platform signature: a debug-signed
# apk installs fine but gets none of the system permissions, and the shell
# fails silently at runtime.
make -C "$PN2_ROOT/vrhome" apk \
    || { echo "FAIL vrhome build"; fail=$((fail+1)); }
BT=$(ls -d "${ANDROID_SDK_ROOT:-/opt/android-sdk}"/build-tools/* | sort -V | tail -1)
for a in vrhome vrhud; do
  "$BT/apksigner" verify --print-certs "$PN2_ROOT/vrhome/out/$a.apk" \
      | grep -q "CN=Android" \
      || { echo "FAIL $a.apk is not platform-signed"; fail=$((fail+1)); }
done
mkd /app/PN2Panels
put "$PN2_ROOT/vrhome/out/vrhome.apk" /app/PN2Panels/PN2Panels.apk 644
mkd /app/PN2Hud
put "$PN2_ROOT/vrhome/out/vrhud.apk" /app/PN2Hud/PN2Hud.apk 644

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
echo "=== OpenXR stack: Turnip Vulkan + Monado runtime ==="
# This replaces the stock VR path for raw OpenXR apps. Verified live:
#   - hwvulkan modules are resolved from /system/lib64/hw as well as /vendor,
#     so Turnip ships in the system image and vendor.img stays untouched.
#   - the Khronos loader falls back to /system/etc/openxr/1/active_runtime.json
#     when no broker app or vendor manifest exists.
#   - pvrservice is the broken compositor behind the black-display bug; its rc
#     is removed so nothing starts it. qvrd stays - it owns the tracking cams.
XR=${PN2_ROOT}/pn2xr
XR_SO="$XR/monado/build-android/src/xrt/targets/openxr/libopenxr_monado.so"
[ -s "$XR/turnip/out/libvulkan_freedreno.so" ] || bash "$XR/turnip/build.sh" \
    || echo "  turnip build failed - put() will report the gap"
[ -s "$XR_SO" ] || bash "$XR/monado/build.sh" \
    || echo "  monado build failed - put() will report the gap"
[ -s "$XR/runtime-apk/out/openxr-runtime.apk" ] || bash "$XR/runtime-apk/build.sh" \
    || echo "  runtime apk build failed - put() will report the gap"
mkd /lib64/hw
put "$XR/turnip/out/libvulkan_freedreno.so" /lib64/hw/vulkan.sdm845.so 644
put "$XR/turnip/out/libc++_shared.so" /lib64/libc++_shared.so 644
mkd /app/MonadoOpenXR
mkd /app/MonadoOpenXR/lib
mkd /app/MonadoOpenXR/lib/arm64
put "$XR/runtime-apk/out/openxr-runtime.apk" /app/MonadoOpenXR/MonadoOpenXR.apk 644
put "$XR_SO" /app/MonadoOpenXR/lib/arm64/libopenxr_monado.so 644
mkd /etc/openxr
mkd /etc/openxr/1
put "$XR/android/active_runtime.json" /etc/openxr/1/active_runtime.json 644
debugfs -w -R "rm /etc/init/pvrservice.rc" "$IMG" >/dev/null 2>&1
echo "  removed /etc/init/pvrservice.rc"

echo
echo "=== repair pass (debugfs write/rm leaves accounting inconsistent) ==="
e2fsck -fy "$IMG" 2>&1 | tail -6

echo
echo "=== fsck must be clean ==="
if e2fsck -fn "$IMG" >/tmp/fsck.txt 2>&1; then
  tail -2 /tmp/fsck.txt; echo "  FILESYSTEM CLEAN"
else
  tail -8 /tmp/fsck.txt; echo "  FILESYSTEM DIRTY"; fail=$((fail+1))
fi

echo
echo "=== free space after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Free blocks'
ls -l "$IMG"
echo
if [ "$fail" -eq 0 ]; then echo "BUILD OK"; else echo "BUILD HAD $fail FAILURES"; fi
echo DONE

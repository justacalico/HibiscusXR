#!/bin/bash
# Install the shared Hibiscus userspace into a system image: the split
# shell (env + HUD), the flutter panel apps, the floating keyboard,
# hsvr OpenXR runtime, then the platform-key re-sign and fsck that make a
# self-consistent image.
#
# Device payloads land in the caller - neo2 stages Pico's stack first
# (267), vmd runs this straight onto the clean base. Everything below is
# identical on every device, so it lives in exactly one place.
#
#   270_shell_stack.sh <image> [logfile]
set -u
. "$(dirname "${BASH_SOURCE[0]}")/../lib/env.sh"
IMG="${1:?usage: $0 <image> [logfile]}"
LOG="${2:-$HIBISCUS_ROOT/notes/270_shell.txt}"
exec >"$LOG" 2>&1

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo "=== free space before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Free blocks|Block size'
ls -l "$IMG"

INIT=${HIBISCUS_ROOT}/overlay/etc/init

echo
echo "=== shell init hooks (generic, not headset hardware) ==="
# hibiscus-home.rc pins the HOME role onto the shell and kills the stock homes;
# hibiscus-openxr.rc stages the runtime into /data/local/tmp/xr at boot;
# hibiscus-adbwifi.rc is opt-in wireless adb. All three are device-independent
# Hibiscus wiring despite the hibiscus_ service names.
put "$INIT/hibiscus-home.rc"    /etc/init/hibiscus-home.rc    644
put "$INIT/hibiscus-openxr.rc"  /etc/init/hibiscus-openxr.rc  644
put "$INIT/hibiscus-adbwifi.rc" /etc/init/hibiscus-adbwifi.rc 644

echo
echo "=== shell split: env (gitlab.neosalsa.home) + HUD (gitlab.neosalsa.hud) ==="
# The env is a platform-signed NativeActivity owning display 0. The HUD is a
# platform-signed service holding a TYPE_SYSTEM_OVERLAY window; it owns the
# virtual displays, so panels live over the env and over summoned VR apps.
# HOME role, hidden API whitelist and disabling the stock Pico homes are
# first-boot work in hibiscus-home.rc - the role holder lives in /data and cannot
# be baked in.
#
# Always rebuild from source and verify the platform signature: a debug-signed
# apk installs fine but gets none of the system permissions, and the shell
# fails silently at runtime.
make -C "$HIBISCUS_ROOT/vrhome" apk \
    || { echo "FAIL vrhome build"; fail=$((fail+1)); }
BT=$(ls -d "${ANDROID_SDK_ROOT:-/opt/android-sdk}"/build-tools/* | sort -V | tail -1)
SIGN_CN=$(openssl x509 -in "$HIBISCUS_ROOT/build/keys/platform.x509.pem" -subject -noout | sed -n 's/.*CN *= *\([^,]*\).*/\1/p')
for a in vrhome vrhud; do
  "$BT/apksigner" verify --print-certs "$HIBISCUS_ROOT/vrhome/out/$a.apk" \
      | grep -q "$SIGN_CN" \
      || { echo "FAIL $a.apk is not platform-signed"; fail=$((fail+1)); }
done
mkd /app/vrhome
put "$HIBISCUS_ROOT/vrhome/out/vrhome.apk" /app/vrhome/vrhome.apk 644
mkd /app/vrhud
put "$HIBISCUS_ROOT/vrhome/out/vrhud.apk" /app/vrhud/vrhud.apk 644

echo
echo "=== shell flutter apps: store ==="
# Same contract as vrhome: built from source here, platform-signed,
# non-uninstallable under /system/app. flutter must be on PATH - the dist
# runner installs the pinned toolchain (manifest.env).
for app in store; do
  command -v flutter >/dev/null 2>&1 || { echo "FAIL flutter not on PATH"; fail=$((fail+1)); break; }
  [ -d "$HIBISCUS_ROOT/$app" ] || { echo "FAIL $HIBISCUS_ROOT/$app not cloned"; fail=$((fail+1)); continue; }
  (cd "$HIBISCUS_ROOT/$app" && flutter build apk --release) \
      || { echo "FAIL $app flutter build"; fail=$((fail+1)); continue; }
  APK="$HIBISCUS_ROOT/$app/build/app/outputs/flutter-apk/app-release.apk"
  "$BT/apksigner" sign --key "$HIBISCUS_ROOT/build/keys/platform.pk8" \
      --cert "$HIBISCUS_ROOT/build/keys/platform.x509.pem" "$APK" \
      || { echo "FAIL $app sign"; fail=$((fail+1)); }
  "$BT/apksigner" verify --print-certs "$APK" | grep -q "$SIGN_CN" \
      || { echo "FAIL $app.apk is not platform-signed"; fail=$((fail+1)); }
done
mkd /app/store
put "$HIBISCUS_ROOT/store/build/app/outputs/flutter-apk/app-release.apk" /app/store/store.apk 644

echo
echo "=== floating keyboard ==="
# The only IME in the image (LatinIME is stripped in 410): a plain java apk
# built by its own Makefile, no flutter. Platform signing isn't required
# for its job - BIND_INPUT_METHOD plus /system/app is all an IME needs -
# but the global re-sign pass below would catch a debug build anyway; the
# Makefile already signs with platform keys when they exist.
[ -d "$HIBISCUS_ROOT/keyboard" ] || { echo "FAIL keyboard not linked"; fail=$((fail+1)); }
make -C "$HIBISCUS_ROOT/keyboard" apk \
    || { echo "FAIL keyboard build"; fail=$((fail+1)); }
mkd /app/keyboard
put "$HIBISCUS_ROOT/keyboard/out/keyboard.apk" /app/keyboard/keyboard.apk 644

echo
echo "=== OpenXR stack: Monado runtime + hsvr kit drivers ==="
# The monado build flattens every drivers/<name>/monado kit driver into
# drv_hsvr and regenerates the driver table - the runtime is identical on
# every device; the detect rules inside each driver pick who wins at boot
# (vmd answers on its host TCP channel, pn2 on sysprops).
#
# Verified on device for the loader paths, which apply to any device:
#   - the Khronos loader searches /product/etc, /odm/etc, /oem/etc,
#     /vendor/etc, /system/etc for openxr/1/active_runtime.json, first hit
#     wins. /product lives inside this image, so our manifest shadows the
#     stock one in /vendor/etc without touching vendor.img.
#   - apps can only dlopen() absolute paths under /data, so hibiscus-openxr.rc
#     stages the runtime into /data/local/tmp/xr at boot and the manifest
#     points there. /system paths are unreachable from an app namespace.
XR=${HIBISCUS_ROOT}/hsvr
XR_SO="$XR/monado/build-android/src/xrt/targets/openxr/libopenxr_monado.so"
# Always run the builds - they are incremental, and skipping on a stale out/
# dir once shipped stale code in the image.
bash "$XR/monado/build.sh" || echo "  monado build failed - put() will report the gap"
bash "$XR/runtime-apk/build.sh" || echo "  runtime apk build failed - put() will report the gap"
mkd /app/MonadoOpenXR
mkd /app/MonadoOpenXR/lib
mkd /app/MonadoOpenXR/lib/arm64
put "$XR/runtime-apk/out/openxr-runtime.apk" /app/MonadoOpenXR/MonadoOpenXR.apk 644
put "$XR_SO" /app/MonadoOpenXR/lib/arm64/libopenxr_monado.so 644
mkd /etc/openxr
mkd /etc/openxr/1
put "$XR/android/active_runtime.json" /etc/openxr/1/active_runtime.json 644
mkd /product/etc/openxr
mkd /product/etc/openxr/1
put "$XR/android/active_runtime.json" /product/etc/openxr/1/active_runtime.json 644
# PM feature flags XR apps query at install/run time (WiVRn marks the first
# two required) - without these the device reads as a plain handset
mkd /etc/permissions
put ${HIBISCUS_ROOT}/overlay/etc/permissions/hibiscus-xr-features.xml /etc/permissions/hibiscus-xr-features.xml 644
# monado is an NDK build and DT_NEEDEDs libc++_shared.so; land it in
# /system/lib64 so system-side loads (and, on neo2, the sphal-linked Turnip
# driver) resolve it
NDK=$(ls -d "${ANDROID_SDK_ROOT:-/opt/android-sdk}"/ndk/* | sort -V | tail -1)
put "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
    /lib64/libc++_shared.so 644

echo
echo "=== re-sign every apk with the real platform key ==="
# The GSI, framework-res and all staged apks carry the public AOSP testkey.
# Re-sign the whole set in place with our platform key so the framework
# cert and every package share one private signature - same model as a
# signed production ROM. /apex is left alone: flattened apexes verify
# against their own apex_pubkey, not the platform cert.
W2="$HIBISCUS_ROOT/notes/resign-work"
rm -rf "$W2"; mkdir -p "$W2"
for d in /app /priv-app /framework /product; do
  debugfs -R "rdump $d $W2" "$IMG" >/dev/null 2>&1
done
mapfile -t APKS < <(cd "$W2" && find . -name '*.apk' -type f)
echo "  ${#APKS[@]} apks to re-sign"
printf '%s\0' "${APKS[@]}" | (cd "$W2" && xargs -0 -P4 -I{} "$BT/apksigner" \
    sign --key "$HIBISCUS_ROOT/build/keys/platform.pk8" \
         --cert "$HIBISCUS_ROOT/build/keys/platform.x509.pem" "{}") \
    || { echo "FAIL apk re-sign pass"; fail=$((fail+1)); }
# one debugfs session writes them all back
CMDS="$W2.cmds"
: > "$CMDS"
IMG_CMDS="$CMDS"
for a in "${APKS[@]}"; do
  rel="${a#./}"
  img_cmd_put "$W2/$rel" "/$rel" 644 || { echo "FAIL re-sign cmdfile"; fail=$((fail+1)); break; }
  printf 'rm %s.idsig\n' "/$rel" >> "$IMG_CMDS"
done
img_run_cmds
for probe in /framework/framework-res.apk /app/vrhud/vrhud.apk /product/priv-app/Settings/Settings.apk; do
  debugfs -R "dump $probe $W2-probe.apk" "$IMG" >/dev/null 2>&1
  "$BT/apksigner" verify --print-certs "$W2-probe.apk" 2>/dev/null | grep -q "$SIGN_CN" \
      || { echo "FAIL $probe not PN2-signed"; fail=$((fail+1)); }
done
rm -rf "$W2" "$CMDS" "$W2-probe.apk"
echo "  re-signed and verified"

echo
echo "=== repair + fsck ==="
img_fsck || fail=$((fail+1))

echo
echo "=== free space after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep -E 'Free blocks'
ls -l "$IMG"
echo
if [ "$fail" -eq 0 ]; then
  echo "SHELL STACK OK"
else
  echo "SHELL STACK HAD $fail FAILURES"
  exit 1
fi

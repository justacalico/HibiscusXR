#!/bin/bash
# Builds Turnip (Mesa freedreno Vulkan) for Android 10 / sdm845 as a
# hwmodule-style driver that drops in as /vendor/lib64/hw/vulkan.sdm845.so.
#
# Needs: meson, ninja, git, and an Android NDK. NDK path comes from
# $ANDROID_NDK_HOME, $ANDROID_NDK, or the newest dir under ~/Android/sdk/ndk.
set -euo pipefail

cd "$(dirname "$0")"
ROOT="$(pwd)"
MESA_TAG="${MESA_TAG:-mesa-25.2.4}"
WORK="${PN2XR_WORK:-$ROOT/.work}"
SRC="$WORK/mesa"
BUILD="$SRC/build-android"
OUT="$ROOT/out"

find_ndk() {
    for v in "$ANDROID_NDK_HOME" "$ANDROID_NDK"; do
        [ -n "$v" ] && [ -d "$v" ] && { echo "$v"; return; }
    done
    ls -d "$HOME"/Android/sdk/ndk/*/ 2>/dev/null | sort -V | tail -1
}

NDK="$(find_ndk)"
[ -n "$NDK" ] || { echo "no Android NDK found"; exit 1; }
NDK="${NDK%/}"
echo "NDK: $NDK"

if [ ! -d "$SRC/.git" ]; then
    mkdir -p "$WORK"
    git clone --depth 1 --branch "$MESA_TAG" \
        https://gitlab.freedesktop.org/mesa/mesa.git "$SRC"
fi

cp "$ROOT/patches/tu_atrace_compat.c" "$SRC/src/freedreno/vulkan/tu_atrace_compat.c"
if ! grep -q tu_atrace_compat "$SRC/src/freedreno/vulkan/meson.build"; then
    patch -d "$SRC" -p1 < "$ROOT/patches/meson-android-compat.patch"
fi

sed "s|@NDK@|$NDK|g" "$ROOT/android-aarch64.cross.in" > "$WORK/android-aarch64.cross"

if [ ! -f "$BUILD/build.ninja" ]; then
    meson setup "$BUILD" "$SRC" \
        --cross-file "$WORK/android-aarch64.cross" \
        --buildtype release \
        -Dplatforms=android \
        -Dplatform-sdk-version=29 \
        -Dandroid-stub=true \
        -Dvulkan-drivers=freedreno \
        -Dfreedreno-kmds=kgsl \
        -Dgallium-drivers= \
        -Dglx=disabled -Degl=disabled -Dgbm=disabled \
        -Dgles1=disabled -Dgles2=disabled \
        -Dopengl=false -Dllvm=disabled \
        -Dlibunwind=disabled -Dzstd=disabled \
        -Dbuild-tests=false -Dtools=
fi

ninja -C "$BUILD" src/freedreno/vulkan/libvulkan_freedreno.so

mkdir -p "$OUT"
cp "$BUILD/src/freedreno/vulkan/libvulkan_freedreno.so" "$OUT/"
for stub in libcutils libhardware liblog libnativewindow libbacktrace libsync; do
    cp "$BUILD/src/android_stub/$stub.so" "$OUT/" 2>/dev/null || true
done
cp "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" "$OUT/"

echo "==> $OUT"
ls -la "$OUT"

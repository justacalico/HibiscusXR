#!/bin/bash
# Reproducible build of the in-process OpenXR runtime (libopenxr_monado.so)
# with the Pico Neo 2 driver.
#
# Clones upstream Monado at a pinned commit, applies the pn2 driver
# registration patch, drops in the driver sources, builds for Android aarch64.
#
# Output: build-android/src/xrt/targets/openxr/libopenxr_monado.so
#
# Requires: Android NDK r27+, cmake, ninja, a local Eigen3 checkout
# (EIGEN3_ROOT or ../eigen relative to this script).

set -e
cd "$(dirname "$0")"
ROOT="$(pwd -P)"

MONADO_REPO="https://gitlab.freedesktop.org/monado/monado.git"
MONADO_COMMIT="09741cbcb45236f4f4f79790ea133cd90d68d5eb"

ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/sdk}"
for root in "$ANDROID_HOME" "$HOME/Android/sdk" /opt/android-sdk; do
    [ -d "$root/ndk" ] || continue
    for ndk in "$root"/ndk/27.* "$root"/ndk/26.*; do
        [ -d "$ndk" ] && NDK="$ndk" && break 2
    done
done
: "${NDK:?no Android NDK 26/27 found}"

# Eigen is headers-only for Monado; fetch it and emit a minimal package config.
EIGEN3_ROOT="${EIGEN3_ROOT:-$(pwd)/../eigen-5.0.0}"
EIGEN_CFG="$(pwd)/../eigen-cmake"
if [ ! -d "$EIGEN3_ROOT/Eigen" ]; then
    echo "fetching eigen3 headers"
    mkdir -p "$EIGEN3_ROOT"
    curl -sL https://gitlab.com/libeigen/eigen/-/archive/5.0.0/eigen-5.0.0.tar.gz \
        | tar xz -C "$EIGEN3_ROOT" --strip-components=1
fi
mkdir -p "$EIGEN_CFG"
cat > "$EIGEN_CFG/Eigen3Config.cmake" <<EOF
if(NOT TARGET Eigen3::Eigen)
  add_library(Eigen3::Eigen INTERFACE IMPORTED)
  set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "$EIGEN3_ROOT")
endif()
set(Eigen3_FOUND TRUE)
set(EIGEN3_FOUND TRUE)
set(Eigen3_VERSION "5.0.0")
set(EIGEN3_VERSION_STRING "5.0.0")
set(EIGEN3_VERSION_MAJOR 5)
set(EIGEN3_VERSION_MINOR 0)
set(EIGEN3_VERSION_PATCH 0)
set(EIGEN3_INCLUDE_DIRS "$EIGEN3_ROOT")
EOF

WORK=_monado-src
if [ ! -d "$WORK/.git" ]; then
    echo "cloning monado"
    git clone --filter=blob:none "$MONADO_REPO" "$WORK"
fi
cd "$WORK"
git fetch -q origin "$MONADO_COMMIT" 2>/dev/null || true
git checkout -q "$MONADO_COMMIT"

echo "applying pn2 patch + driver"
git checkout -q . # drop local edits so the patch applies cleanly
git apply "$ROOT/patches/pn2-driver-registration.patch"
mkdir -p src/xrt/drivers/pn2
cp "$ROOT"/driver/pn2/*.c "$ROOT"/driver/pn2/*.h src/xrt/drivers/pn2/

# the controller sharemem decoder is shared with the vrhome dash - one
# implementation, copied in at build time so there is no duplicated layout
CTRL_STATE_SRC="${PN2_VRHOME:-$ROOT/../../vrhome}/src/input"
for f in ctrl_state.c ctrl_state.h; do
    if [ ! -f "$CTRL_STATE_SRC/$f" ]; then
        echo "missing shared decoder $CTRL_STATE_SRC/$f (set PN2_VRHOME)" >&2
        exit 1
    fi
    cp "$CTRL_STATE_SRC/$f" src/xrt/drivers/pn2/
done
cd ..

mkdir -p "$ROOT/build-android"
cd "$ROOT/build-android"
cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DANDROID_STL=c++_static \
    -DXRT_FEATURE_AHARDWARE_BUFFER=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DXRT_HAVE_WAYLAND=OFF -DXRT_HAVE_XLIB=OFF -DXRT_HAVE_XCB=OFF \
    -DXRT_FEATURE_SERVICE=OFF -DXRT_FEATURE_SERVICE_SYSTEMD=OFF \
    -DXRT_FEATURE_STEAMVR_PLUGIN=OFF \
    -DEigen3_DIR="$EIGEN_CFG" \
    "$ROOT/$WORK"
ninja openxr_monado

echo "built: $(pwd)/src/xrt/targets/openxr/libopenxr_monado.so"

#!/bin/bash
# Build the Linux OpenXR runtime: Monado plus drv_hsvc, the driver that
# serves whatever the hsvc bus publishes as OpenXR devices. This is the
# link that lets the shell and any OpenXR app run on the distro - the
# bus is the device contract, monado is the runtime.
#
#   xr/build-runtime.sh                 build into out/xr
#   MONADO_SRC=<checkout>               use an existing monado tree
#                                       (must already carry the hsvr
#                                       registration patch, like the
#                                       vendored _monado-src)
#
# Requires: cmake, ninja, a C toolchain, vulkan headers, Eigen3 (the
# repo's stub config at system/hsvr/eigen-cmake covers it when
# eigen-5.0.0 is checked out next to it).
#
# Output (out/xr/): libopenxr_monado.so, monado-service, monado-cli and
# a runtime manifest. mkimage.sh -x stages these into the image.

set -e
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)
LINUX="$ROOT/system/linux"
OUT="$LINUX/out/xr"

MONADO_REPO="https://gitlab.freedesktop.org/monado/monado.git"
MONADO_COMMIT="09741cbcb45236f4f4f79790ea133cd90d68d5eb"

SRC="${MONADO_SRC:-$ROOT/system/hsvr/monado/_monado-src}"
if [ ! -d "$SRC/src/xrt" ]; then
	SRC="$LINUX/out/monado-src"
	if [ ! -d "$SRC/src/xrt" ]; then
		echo "cloning monado @ $MONADO_COMMIT"
		git clone --filter=blob:none "$MONADO_REPO" "$SRC"
		git -C "$SRC" checkout "$MONADO_COMMIT"
	fi
	echo "applying hsvr registration + hsvc driver patches"
	git -C "$SRC" apply "$ROOT/system/hsvr/monado/patches/hsvr-driver-registration.patch"
	git -C "$SRC" apply "$LINUX/xr/patches/hsvc-driver.patch"
fi

# our patch is written against the tree the hsvr kit patch produces;
# when the source already has it (vendored checkout) apply only ours
if ! git -C "$SRC" apply --check "$LINUX/xr/patches/hsvc-driver.patch" \
	>/dev/null 2>&1; then
	echo "hsvc driver patch already applied or does not apply to $SRC" >&2
else
	git -C "$SRC" apply "$LINUX/xr/patches/hsvc-driver.patch"
fi

# drop the driver sources + the libhsvc client into the driver dir; the
# cmake glob picks them up
DRV="$SRC/src/xrt/drivers/hsvc"
mkdir -p "$DRV/hsvc"
cp "$LINUX"/xr/hsvc/*.c "$LINUX"/xr/hsvc/*.h "$DRV/"
cp "$LINUX/src/hsvc.c" "$DRV/"
cp "$LINUX"/include/hsvc/*.h "$DRV/hsvc/"

# eigen: reuse the repo's stub config + checkout when present, else
# write one under out/
EIGEN3_ROOT="${EIGEN3_ROOT:-$ROOT/system/hsvr/eigen-5.0.0}"
EIGEN_CFG="$ROOT/system/hsvr/eigen-cmake"
if [ ! -f "$EIGEN_CFG/Eigen3Config.cmake" ]; then
	EIGEN_CFG="$LINUX/out/eigen-cmake"
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
fi
if [ ! -d "$EIGEN3_ROOT/Eigen" ]; then
	echo "fetching eigen3 headers"
	mkdir -p "$EIGEN3_ROOT"
	curl -sL https://gitlab.com/libeigen/eigen/-/archive/5.0.0/eigen-5.0.0.tar.gz \
	    | tar xz -C "$EIGEN3_ROOT" --strip-components=1
fi

FLAGS=(
	-DEigen3_DIR="$EIGEN_CFG"
	-DCMAKE_BUILD_TYPE=Release
	-DXRT_BUILD_DRIVER_HSVC=ON
	-DBUILD_TESTING=OFF
	-DXRT_MODULE_MONADO_GUI=OFF
	-DXRT_MODULE_OPENVR_STATE_TRACKER=OFF
	-DXRT_MODULE_MERCURY_HANDTRACKING=OFF
	-DXRT_FEATURE_STEAMVR_PLUGIN=OFF
	-DXRT_FEATURE_NO_COMPOSITOR_FOR_HEADLESS_SESSIONS=ON
	-DXRT_HAVE_SDL2=OFF
	-DXRT_HAVE_OPENCV=OFF
	-DXRT_FEATURE_SLAM=OFF
)
# every stock hardware driver off: the bus is the only device contract.
# SIMULATED stays as the fallback when nothing serves the bus.
for d in ANDROID HSVR ARDUINO BLUBUR_S1 CONTACTGLOVE DAYDREAM DEPTHAI \
	EUROC HANDTRACKING HDK HYDRA ILLIXR NS OHMD OPENGLOVES PSMV PSSENSE \
	PSVR PSVR2 QWERTY REALSENSE REMOTE RIFT RIFT_SENSOR RIFT_S ROKID \
	STEAMVR_LIGHTHOUSE SURVIVE UVC ULV2 ULV5 V4L2 VF VIVE WMR XREAL_AIR \
	SIMULAVR SOLARXR TWRAP; do
	FLAGS+=(-DXRT_BUILD_DRIVER_$d=OFF)
done

BUILD="$OUT/build"
cmake -S "$SRC" -B "$BUILD" -G Ninja "${FLAGS[@]}"
ninja -C "$BUILD" openxr_monado monado-service monado-cli

mkdir -p "$OUT/lib" "$OUT/bin" "$OUT/share/openxr/1"
cp "$BUILD/src/xrt/targets/openxr/libopenxr_monado.so" "$OUT/lib/"
cp "$BUILD/src/xrt/targets/service/monado-service" "$OUT/bin/"
cp "$BUILD/src/xrt/targets/cli/monado-cli" "$OUT/bin/"
cat > "$OUT/share/openxr/1/active_runtime.json" <<'EOF'
{
    "file_format_version": "1.0.0",
    "runtime": {
        "name": "Monado",
        "library_path": "/usr/lib/libopenxr_monado.so"
    }
}
EOF
echo "xr: $OUT (lib/libopenxr_monado.so bin/monado-service bin/monado-cli)"

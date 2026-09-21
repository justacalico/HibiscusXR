#!/bin/bash
# Build the shim .so on Linux - same outputs as build.ps1 / build_air.ps1, but
# against a host NDK instead of a Windows toolchain.
#
#   libshim_pvr.so   - the two libgui symbols Q removed (pvrservice LD_PRELOAD)
#   libshim_air.so   - libgui/libui ABI shims for the 8.1 passthrough stack
#   libskia_stub.so  - empty libskia.so stand-in for libaircamera's dead DT_NEEDED
#
# The link needs the device's own Q framework libs (they carry the SONAMEs lld
# records and the forward targets). They are not ours; if notes/qlibs is empty
# they are pulled off the connected device (which runs the GSI) first.
set -euo pipefail
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
Q="${Q:-$PN2_ROOT/notes/qlibs}"
mkdir -p "$Q"

# NDK clang: honour NDK/ANDROID_NDK, else newest ndk under the SDK.
NDK="${NDK:-${ANDROID_NDK:-}}"
if [ -z "$NDK" ]; then
  NDK=$(ls -d "${ANDROID_HOME:-$HOME/Android/Sdk}"/ndk/*/ 2>/dev/null | sort -V | tail -1)
fi
PRE="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
CC="$PRE/aarch64-linux-android29-clang"
CXX="$PRE/aarch64-linux-android29-clang++"
[ -x "$CC" ] || { echo "no NDK clang at $CC - set NDK="; exit 1; }

# Q framework libs the shims link against. Pull from the device if absent.
need="libgui libui libutils libcamera_client libtinyxml2"
for l in $need; do
  [ -s "$Q/$l.so" ] || adb pull "/system/lib64/$l.so" "$Q/$l.so" >/dev/null 2>&1 || true
done
[ -s "$Q/libgui.so" ] || { echo "no $Q/libgui.so - connect the device or drop the Q libs in"; exit 1; }

cd "$(dirname "$0")"

echo "=== libshim_pvr.so ==="
"$CC" -shared -O2 -Wl,--allow-shlib-undefined -Wl,-soname,libshim_pvr.so \
      -o libshim_pvr.so shim_pvr.S shim_events.c "$Q/libgui.so" -ldl -llog

echo "=== libskia_stub.so (soname libskia.so) ==="
"$CC" -shared -O2 -Wl,-soname,libskia.so -o libskia_stub.so stub_skia.c

echo "=== libshim_air.so ==="
"$CXX" -shared -O2 -fno-exceptions -fno-rtti -nostdlib++ \
      -Wl,--allow-shlib-undefined -Wl,-soname,libshim_air.so \
      -o libshim_air.so shim_air.cpp \
      "$Q/libgui.so" "$Q/libui.so" "$Q/libutils.so" "$Q/libcamera_client.so" "$Q/libtinyxml2.so" -llog

# libc++_shared would break every process it is preloaded into - assert it is gone.
if readelf -dW libshim_air.so | grep -q 'libc++_shared'; then
  echo "FAIL: libshim_air.so still needs libc++_shared" >&2; exit 1
fi

echo "built: $(ls -l libshim_pvr.so libskia_stub.so libshim_air.so | awk '{print $NF"("$5")"}' | tr '\n' ' ')"

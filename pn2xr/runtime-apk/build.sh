#!/bin/bash
# Build the system-discoverable OpenXR runtime APK.
#
# The Khronos loader (libopenxr_loader.so inside apps) finds the runtime via
# the OpenXRRuntimeService intent + SoFilename metadata, dlopens
# libopenxr_monado.so from this package, and the runtime in turn loads the
# MonadoView/NativeCounterpart java helpers from this same APK (dladdr).
#
# Usage:
#   ./build.sh /path/to/libopenxr_monado.so        -> out/openxr-runtime.apk
#   ./build.sh --install /path/to/libopenxr_monado.so  -> build + adb install

set -e
cd "$(dirname "$0")"

for root in "$ANDROID_HOME" "$HOME/Android/sdk" /opt/android-sdk; do
    [ -d "$root" ] || continue
    [ -z "$BT" ] && [ -d "$root/build-tools/35.0.0" ] && BT="$root/build-tools/35.0.0"
    [ -z "$PLATFORM_JAR" ] && [ -f "$root/platforms/android-35/android.jar" ] && PLATFORM_JAR="$root/platforms/android-35/android.jar"
    for n in "$root"/ndk/27.* "$root"/ndk/26.*; do
        [ -z "$NDK" ] && [ -d "$n" ] && NDK="$n"
    done
done
MINAPI=26

RUNTIME_SO=""
DO_INSTALL=0
for arg in "$@"; do
    case "$arg" in
        --install) DO_INSTALL=1 ;;
        *) RUNTIME_SO="$arg" ;;
    esac
done

if [ -z "$RUNTIME_SO" ]; then
    RUNTIME_SO="../monado/build-android/src/xrt/targets/openxr/libopenxr_monado.so"
fi
[ -f "$RUNTIME_SO" ] || { echo "runtime .so not found: $RUNTIME_SO" >&2; exit 1; }
RUNTIME_SO="$(cd "$(dirname "$RUNTIME_SO")" && pwd)/$(basename "$RUNTIME_SO")"

OUT=out
rm -rf "$OUT"
mkdir -p "$OUT/classes" "$OUT/apk/lib/arm64-v8a"

echo "[1/4] java classes"
javac -source 8 -target 8 -classpath "$PLATFORM_JAR" \
    -d "$OUT/classes" \
    ../java/org/freedesktop/monado/auxiliary/*.java \
    ../java/org/freedesktop/monado/android_common/*.java

echo "[2/4] dex"
"$BT/d8" --lib "$PLATFORM_JAR" --min-api "$MINAPI" \
    --output "$OUT" $(find "$OUT/classes" -name "*.class")
mv "$OUT/classes.dex" "$OUT/apk/classes.dex"

echo "[3/4] apk"
cp "$RUNTIME_SO" "$OUT/apk/lib/arm64-v8a/libopenxr_monado.so"
cp "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
    "$OUT/apk/lib/arm64-v8a/"
"$BT/aapt2" link -o "$OUT/runtime-unsigned.apk" \
    -I "$PLATFORM_JAR" \
    --manifest AndroidManifest.xml \
    --min-sdk-version "$MINAPI" --target-sdk-version "$MINAPI"
cd "$OUT/apk"
zip -q -r "../runtime-unsigned.apk" classes.dex lib
cd ../..

echo "[4/4] sign"
"$BT/apksigner" sign --ks "$HOME/.android/debug.keystore" \
    --ks-pass pass:android --out out/openxr-runtime.apk out/runtime-unsigned.apk

echo "built out/openxr-runtime.apk"

if [ "$DO_INSTALL" = 1 ]; then
    adb install -r out/openxr-runtime.apk
fi

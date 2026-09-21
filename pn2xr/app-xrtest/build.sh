#!/bin/bash
# Build the xrtest OpenXR test APK.
#
# Self-contained: NativeActivity + bundled libopenxr_monado.so + MonadoView
# java helpers loaded by the runtime via dladdr on its own apk.
#
# Usage:
#   ./build.sh /path/to/libopenxr_monado.so        -> out/xrtest.apk
#   ./build.sh --install /path/to/libopenxr_monado.so  -> build + adb install

set -e
cd "$(dirname "$0")"

ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/sdk}"
# SDK pieces may be split across roots; probe each.
for root in "$ANDROID_HOME" "$HOME/Android/sdk" /opt/android-sdk; do
    [ -d "$root" ] || continue
    [ -z "$NDK" ] && [ -d "$root/ndk/27.1.12297006" ] && NDK="$root/ndk/27.1.12297006"
    [ -z "$BT" ] && [ -d "$root/build-tools/35.0.0" ] && BT="$root/build-tools/35.0.0"
    [ -z "$PLATFORM_JAR" ] && [ -f "$root/platforms/android-35/android.jar" ] && PLATFORM_JAR="$root/platforms/android-35/android.jar"
done
NDK="${ANDROID_NDK_HOME:-$NDK}"
MINAPI=29

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
if [ ! -f "$RUNTIME_SO" ]; then
    echo "runtime .so not found: $RUNTIME_SO" >&2
    echo "build monado first (monado/build.sh)" >&2
    exit 1
fi
RUNTIME_SO="$(cd "$(dirname "$RUNTIME_SO")" && pwd)/$(basename "$RUNTIME_SO")"

OUT=out
CLANG="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$MINAPI-clang"
rm -rf "$OUT"
mkdir -p "$OUT/classes" "$OUT/apk/lib/arm64-v8a"

echo "[1/5] native lib"
"$CLANG" -O2 -fPIC -shared \
    -I include -I "$NDK/sources/android/native_app_glue" \
    native/main.c "$NDK/sources/android/native_app_glue/android_native_app_glue.c" \
    -o "$OUT/apk/lib/arm64-v8a/libxrtest.so" \
    -landroid -llog -lEGL -lGLESv2 -ldl

echo "[2/5] java classes"
javac -source 8 -target 8 -classpath "$PLATFORM_JAR" \
    -d "$OUT/classes" \
    ../java/org/freedesktop/monado/auxiliary/*.java

echo "[3/5] dex"
"$BT/d8" --lib "$PLATFORM_JAR" --min-api "$MINAPI" \
    --output "$OUT" "$OUT"/classes/org/freedesktop/monado/auxiliary/*.class
mv "$OUT/classes.dex" "$OUT/apk/classes.dex"

echo "[4/5] apk"
cp "$RUNTIME_SO" "$OUT/apk/lib/arm64-v8a/libopenxr_monado.so"
cp "$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so" \
    "$OUT/apk/lib/arm64-v8a/"
"$BT/aapt2" link -o "$OUT/xrtest-unsigned.apk" \
    -I "$PLATFORM_JAR" \
    --manifest AndroidManifest.xml \
    --min-sdk-version "$MINAPI" --target-sdk-version "$MINAPI"
cd "$OUT/apk"
zip -q -r "../xrtest-unsigned.apk" classes.dex lib
cd ../..

echo "[5/5] sign"
"$BT/apksigner" sign --ks "$HOME/.android/debug.keystore" \
    --ks-pass pass:android --out out/xrtest.apk out/xrtest-unsigned.apk

echo "built out/xrtest.apk"

if [ "$DO_INSTALL" = 1 ]; then
    adb install -r out/xrtest.apk
fi

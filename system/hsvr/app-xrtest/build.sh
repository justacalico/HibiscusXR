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
# SDK pieces may be split across roots; probe each. Prefer the pinned
# versions, fall back to the newest installed. The glob has to expand in
# the caller's word, so $1 stays unquoted here.
newest() { ls -d $1 2>/dev/null | sort -V | tail -1; }
for root in "$ANDROID_HOME" "$HOME/Android/sdk" /opt/android-sdk; do
    [ -d "$root" ] || continue
    [ -z "$NDK" ] && [ -d "$root/ndk/27.1.12297006" ] && NDK="$root/ndk/27.1.12297006"
    [ -z "$NDK" ] && NDK="$(newest "$root/ndk/*")"
    [ -z "$BT" ] && [ -d "$root/build-tools/35.0.0" ] && BT="$root/build-tools/35.0.0"
    [ -z "$BT" ] && BT="$(newest "$root/build-tools/*")"
    [ -z "$PLATFORM_JAR" ] && [ -f "$root/platforms/android-35/android.jar" ] && PLATFORM_JAR="$root/platforms/android-35/android.jar"
    [ -z "$PLATFORM_JAR" ] && PLATFORM_JAR="$(newest "$root/platforms/android-*/android.jar")"
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
    native/*.c "$NDK/sources/android/native_app_glue/android_native_app_glue.c" \
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

# Stock Khronos loader: the app links this like any normal OpenXR app and it
# finds the system runtime on its own. Pinned + checksummed, cached under
# prebuilt/ so rebuilds stay offline.
LOADER_SO=prebuilt/libopenxr_loader.so
LOADER_URL="https://repo1.maven.org/maven2/org/khronos/openxr/openxr_loader_for_android/1.1.48/openxr_loader_for_android-1.1.48.aar"
LOADER_SHA="fcca3faa670dde96d9b06fb525d0261f56cd111e2db3898cb2dd61fe93c5455a"
if [ ! -f "$LOADER_SO" ]; then
    mkdir -p prebuilt
    curl -fsSL -o /tmp/openxr_loader.aar "$LOADER_URL"
    echo "$LOADER_SHA  /tmp/openxr_loader.aar" | sha256sum -c
    unzip -o -j /tmp/openxr_loader.aar jni/arm64-v8a/libopenxr_loader.so -d prebuilt
fi
cp "$LOADER_SO" "$OUT/apk/lib/arm64-v8a/libopenxr_loader.so"
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

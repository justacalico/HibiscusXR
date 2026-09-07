#!/bin/bash
# Build vrhome.apk without Gradle: aapt2 -> inject .so -> zipalign -> apksigner.
#
# Env overrides:
#   NDK      Android NDK dir          (default: newest under $ANDROID_SDK_ROOT/ndk)
#   BT       build-tools dir          (default: newest under $ANDROID_SDK_ROOT/build-tools)
#   JAR      platform android.jar     (default: android-33)
#   KS       signing keystore         (default: ./debug.keystore, generated once)
set -euo pipefail

SDK=${ANDROID_SDK_ROOT:-/opt/android-sdk}
NDK=${NDK:-$(ls -d "$SDK"/ndk/* 2>/dev/null | sort -V | tail -1)}
BT=${BT:-$(ls -d "$SDK"/build-tools/* 2>/dev/null | sort -V | tail -1)}
JAR=${JAR:-$SDK/platforms/android-33/android.jar}
KS=${KS:-$(dirname "$0")/debug.keystore}
SRC=$(dirname "$0")
OUT="$SRC/out"

CLANG="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang"
GLUE="$NDK/sources/android/native_app_glue"

for p in "$CLANG" "$GLUE/android_native_app_glue.c" "$JAR" "$BT/aapt2"; do
    [ -e "$p" ] || { echo "missing: $p" >&2; exit 1; }
done
mkdir -p "$OUT/lib/arm64-v8a"

echo "[1/5] compile glue + native lib"
# glue is C; building it with clang++ miscompiles the implicit void* conversions
"$CC" -c -fPIC -O2 -I "$GLUE" -o "$OUT/glue.o" "$GLUE/android_native_app_glue.c"
"$CLANG" -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
    -I "$GLUE" -o "$OUT/lib/arm64-v8a/libvrhome.so" \
    "$SRC/src/main.cpp" "$OUT/glue.o" \
    -landroid -lEGL -lGLESv2 -llog -lm -u ANativeActivity_onCreate

echo "[2/5] aapt2 link"
"$BT/aapt2" link -I "$JAR" \
    --manifest "$SRC/AndroidManifest.xml" \
    --min-sdk-version 26 --target-sdk-version 29 \
    -o "$OUT/unsigned.apk"

echo "[3/5] inject native lib"
cp "$OUT/unsigned.apk" "$OUT/unaligned.apk"
# aapt2 produces a zip we can extend in place
python3 - "$OUT/unaligned.apk" "$OUT/lib/arm64-v8a/libvrhome.so" <<'PY'
import sys, zipfile
apk, so = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(apk, "a", zipfile.ZIP_STORED) as z:
    z.writestr("lib/arm64-v8a/libvrhome.so", open(so, "rb").read())
PY

echo "[4/5] zipalign"
"$BT/zipalign" -f 4 "$OUT/unaligned.apk" "$OUT/vrhome.apk"

echo "[5/5] sign"
if [ ! -f "$KS" ]; then
    keytool -genkeypair -v -keystore "$KS" -alias vrhome \
        -keyalg RSA -keysize 2048 -validity 10000 \
        -storepass android -keypass android \
        -dname "CN=vrhome,O=pn2,C=CN" 2>/dev/null
fi
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android "$OUT/vrhome.apk"
"$BT/apksigner" verify --print-certs "$OUT/vrhome.apk" | head -2

echo "built: $OUT/vrhome.apk ($(stat -c%s "$OUT/vrhome.apk") bytes)"

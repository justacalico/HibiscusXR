#!/bin/bash
# Build vrhome.apk without Gradle: javac -> d8 -> aapt2 -> inject .so+dex ->
# zipalign -> apksigner.
#
# Env overrides:
#   NDK      Android NDK dir          (default: newest under $ANDROID_SDK_ROOT/ndk)
#   BT       build-tools dir          (default: newest under $ANDROID_SDK_ROOT/build-tools)
#   JAR      platform android.jar     (default: android-33)
#   KEYS     signing key dir          (default: ../build/keys - platform.pk8 +
#                                       platform.x509.pem; falls back to a
#                                       generated debug keystore)
set -euo pipefail

SDK=${ANDROID_SDK_ROOT:-/opt/android-sdk}
NDK=${NDK:-$(ls -d "$SDK"/ndk/* 2>/dev/null | sort -V | tail -1)}
BT=${BT:-$(ls -d "$SDK"/build-tools/* 2>/dev/null | sort -V | tail -1)}
JAR=${JAR:-$SDK/platforms/android-29/android.jar}
KEYS=${KEYS:-$(dirname "$0")/../build/keys}
SRC=$(dirname "$0")
OUT="$SRC/out"

CLANG="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang"
GLUE="$NDK/sources/android/native_app_glue"
D8="$BT/d8"

for p in "$CLANG" "$GLUE/android_native_app_glue.c" "$JAR" "$BT/aapt2" "$D8"; do
    [ -e "$p" ] || { echo "missing: $p" >&2; exit 1; }
done
mkdir -p "$OUT/lib/arm64-v8a" "$OUT/classes" "$OUT/dex"

echo "[1/6] compile glue + native lib"
# glue is C; building it with clang++ miscompiles the implicit void* conversions
"$CC" -c -fPIC -O2 -I "$GLUE" -o "$OUT/glue.o" "$GLUE/android_native_app_glue.c"
"$CLANG" -shared -fPIC -O2 -std=c++17 -static-libstdc++ \
    -I "$GLUE" -o "$OUT/lib/arm64-v8a/libvrhome.so" \
    "$SRC"/src/*.cpp "$SRC"/src/*/*.cpp "$OUT/glue.o" \
    -landroid -lEGL -lGLESv2 -llog -lm -u ANativeActivity_onCreate

echo "[2/6] javac"
javac -source 8 -target 8 -cp "$JAR" \
    -d "$OUT/classes" "$SRC"/java/org/pn2/vrhome/*.java

echo "[3/6] d8"
rm -f "$OUT/dex/classes.dex"
"$D8" --classpath "$JAR" --output "$OUT/dex" "$OUT"/classes/org/pn2/vrhome/*.class

echo "[4/6] aapt2 link + inject lib/dex"
"$BT/aapt2" link -I "$JAR" \
    --manifest "$SRC/AndroidManifest.xml" \
    --min-sdk-version 26 --target-sdk-version 29 \
    -o "$OUT/unaligned.apk"
python3 - "$OUT/unaligned.apk" "$OUT/lib/arm64-v8a/libvrhome.so" "$OUT/dex/classes.dex" <<'PY'
import sys, zipfile
apk, so, dex = sys.argv[1], sys.argv[2], sys.argv[3]
with zipfile.ZipFile(apk, "a", zipfile.ZIP_STORED) as z:
    z.writestr("lib/arm64-v8a/libvrhome.so", open(so, "rb").read())
    z.writestr("classes.dex", open(dex, "rb").read())
PY

echo "[5/6] zipalign"
"$BT/zipalign" -f 4 "$OUT/unaligned.apk" "$OUT/vrhome.apk"

echo "[6/6] sign"
if [ -f "$KEYS/platform.pk8" ] && [ -f "$KEYS/platform.x509.pem" ]; then
    "$BT/apksigner" sign --key "$KEYS/platform.pk8" \
        --cert "$KEYS/platform.x509.pem" "$OUT/vrhome.apk"
    echo "signed with platform key"
else
    KS=${KS:-$SRC/debug.keystore}
    if [ ! -f "$KS" ]; then
        keytool -genkeypair -v -keystore "$KS" -alias vrhome \
            -keyalg RSA -keysize 2048 -validity 10000 \
            -storepass android -keypass android \
            -dname "CN=vrhome,O=pn2,C=CN" 2>/dev/null
    fi
    "$BT/apksigner" sign --ks "$KS" --ks-pass pass:android "$OUT/vrhome.apk"
    echo "signed with debug key (system perms will NOT be granted)"
fi
"$BT/apksigner" verify --print-certs "$OUT/vrhome.apk" | head -2

echo "built: $OUT/vrhome.apk ($(stat -c%s "$OUT/vrhome.apk") bytes)"

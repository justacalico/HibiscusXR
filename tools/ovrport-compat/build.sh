#!/bin/sh
set -e
cd "$(dirname "$0")"

NDK="${ANDROID_NDK_HOME:-$HOME/android-sdk/ndk/27.1.12297006}"
API="${API:-24}"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$API-clang"

"$CC" -shared -fPIC -O2 -Wall \
    -o libhsvrovrp.so src/hsvr_ovrp_compat.c \
    -llog -lEGL -ldl

echo "built libhsvrovrp.so"

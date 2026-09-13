#!/bin/bash
# Build + run the host unit tests. Only the pure modules are compiled - they
# carry no Android/GL includes, so the same sources that ship in the APK run
# here natively.
set -euo pipefail

SRC=$(dirname "$0")
CXX=${CXX:-c++}
BIN="$SRC/out/tests/runtests"

mkdir -p "$SRC/out/tests"
"$CXX" -std=c++17 -O0 -g -Wall -Wextra -I "$SRC/src" -o "$BIN" \
    "$SRC"/tests/*.cpp \
    "$SRC/src/math/mat4.cpp" \
    "$SRC/src/math/head.cpp" \
    "$SRC/src/panels/layout.cpp" \
    "$SRC/src/text/utf8.cpp" \
    "$SRC/src/text/glyphs.cpp" \
    "$SRC/src/render/scene_geo.cpp" \
    "$SRC/src/input/keys.cpp" \
    -lm
"$BIN"

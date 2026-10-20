#!/usr/bin/env bash
# Build the freestanding vmd guest HALs (gralloc.vmd.so, hwcomposer.vmd.so)
# for aarch64 with the host cross toolchain. They have no DT_NEEDED at
# all, so the vendor passthrough services can dlopen them in any process.
#
# usage: vmd/hal/build-hal.sh [OUTDIR]   (default: vmd/hal/out)
set -euo pipefail
cd "$(dirname "$0")"

CC=${VMD_HAL_CC:-aarch64-linux-gnu-gcc}
command -v "$CC" >/dev/null || {
    echo "build-hal: $CC not found (install gcc-aarch64-linux-gnu)" >&2
    exit 1
}

OUT=${1:-out}
mkdir -p "$OUT"

CFLAGS="-O2 -fPIC -shared -nostdlib -ffreestanding -fno-stack-protector -Wall -Wextra"
for m in gralloc hwcomposer; do
    "$CC" $CFLAGS -o "$OUT/$m.vmd.so" "$m""_vmd.c"
done
echo "build-hal: wrote $OUT/gralloc.vmd.so $OUT/hwcomposer.vmd.so"

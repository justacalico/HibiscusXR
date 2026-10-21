#!/usr/bin/env bash
# Build and run the DRM resource-count clamp regression tests. The
# test binaries are freestanding aarch64 like the code under test and
# run on the host through qemu-aarch64, with the DRM ioctls faked by a
# hostile kernel (the virtio-gpu host controls these counts).
#
# usage: vmd/tests/run.sh   (needs aarch64-linux-gnu-gcc and
# qemu-aarch64 or qemu-aarch64-static, or QEMU=<path>)
set -euo pipefail
cd "$(dirname "$0")/.."

CC=${VMD_HAL_CC:-aarch64-linux-gnu-gcc}
QEMU=${QEMU:-$(command -v qemu-aarch64 || command -v qemu-aarch64-static || true)}
[ -n "$QEMU" ] || { echo "run.sh: need qemu-aarch64 (or set QEMU=)" >&2; exit 1; }
command -v "$CC" >/dev/null || { echo "run.sh: $CC not found" >&2; exit 1; }

OUT=tests/out
mkdir -p "$OUT"
CFLAGS="-O2 -nostdlib -ffreestanding -fno-stack-protector -Wall -Wextra -static"

# drmpaint carries its own ioct/open/entry symbols, so its call sites
# get rewired to the test hooks first.
sed -e 's/ioct(fd/tst_ioct(fd/g' \
    -e 's/fd=fd_open(/fd=tst_open(/' \
    -e 's/\bmain(/drmpaint_main(/g' \
    -e 's/_start/drmpaint_start/g' \
    bootenv/tools/drmpaint.c > "$OUT/drmpaint_hooks.c"

"$CC" $CFLAGS -o "$OUT/test_hwc_clamp" tests/test_hwc_clamp.c
"$CC" $CFLAGS -Wno-unused-function -Wno-sign-compare -I/usr/include \
    -o "$OUT/test_drmpaint_clamp" tests/test_drmpaint_clamp.c

"$QEMU" "$OUT/test_hwc_clamp"
"$QEMU" "$OUT/test_drmpaint_clamp"
echo "drm clamp tests passed"

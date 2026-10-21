#!/usr/bin/env bash
# Build and run the vmd regression tests.
#
# test_chan_stop is hosted POSIX (vmd_chan.c uses libc sockets), so it
# builds with the native compiler and runs directly.
#
# The DRM resource-count clamp tests are freestanding aarch64 like the
# code under test and run on the host through qemu-aarch64, with the DRM
# ioctls faked by a hostile kernel (the virtio-gpu host controls these
# counts). They are skipped when the cross toolchain or qemu is missing.
#
# usage: vmd/tests/run.sh   (freestanding tests need aarch64-linux-gnu-gcc
# and qemu-aarch64 or qemu-aarch64-static, or QEMU=<path>)
set -euo pipefail
cd "$(dirname "$0")/.."

OUT=tests/out
mkdir -p "$OUT"

"${HOSTCC:-cc}" -O2 -Wall -Wextra -D_GNU_SOURCE \
    -I../drivers/vmd/proto -I../drivers/vmd/monado \
    -o "$OUT/test_chan_stop" tests/test_chan_stop.c \
    ../drivers/vmd/monado/vmd_chan.c -lpthread
"$OUT/test_chan_stop"

CC=${VMD_HAL_CC:-aarch64-linux-gnu-gcc}
QEMU=${QEMU:-$(command -v qemu-aarch64 || command -v qemu-aarch64-static || true)}
if [ -z "$QEMU" ] || ! command -v "$CC" >/dev/null; then
  echo "run.sh: skipping freestanding tests (need aarch64-linux-gnu-gcc and qemu-aarch64)" >&2
  exit 0
fi

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

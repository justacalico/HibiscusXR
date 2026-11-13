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

# embedded guest bundle: trailer probe + guest.env parsing, then a
# round trip - embed a fake guest set into a real binary and have
# -guestinfo find it. Needs the glfw/epoxy deps for the make, so it
# skips where the tool itself cannot build.
"${CXX:-c++}" -O2 -Wall -Wextra -std=c++17 -Isrc -I../drivers/vmd/proto \
    -o "$OUT/test_guest" tests/test_guest.cpp src/guest.cpp
"$OUT/test_guest"

# the mouse -> wand mapping is pure, like the in-guest vmdmap.h was
"${CXX:-c++}" -O2 -Wall -Wextra -std=c++17 -Isrc -I../drivers/vmd/proto \
    -o "$OUT/test_mousemap" tests/test_mousemap.cpp
"$OUT/test_mousemap"

# the hsvc service endpoint: loopback client speaks HELLO/INFO/SUB and
# reads hsvc_pose + hsvc_ctrl frames off the wire
"${CXX:-c++}" -O2 -Wall -Wextra -std=c++17 -Isrc -I../drivers/vmd/proto \
    -I../system/linux/include \
    -o "$OUT/test_svc" tests/test_svc.cpp src/svc.cpp -lpthread
"$OUT/test_svc"

# guest-user.sh: a staged root gets the hibiscus account baked in plus
# the console getty autologin drop-in, so the guest never sits on a
# login prompt
fk="$OUT/fakeroot"
mkdir -p "$fk/etc" "$fk/bin"
printf 'root:x:0:0:root:/root:/bin/sh\ndaemon:x:1:1:d:/:/bin/sh\n' \
    > "$fk/etc/passwd"
printf 'root:x:0:\ndaemon:x:1:\nvideo:x:39:\ninput:x:104:\n' \
    > "$fk/etc/group"
printf 'root:!:19000:0:99999:7:::\n' > "$fk/etc/shadow"
bash vm/guest-user.sh "$fk"
grep -q '^hibiscus:x:1000:1000:Hibiscus:/home/hibiscus:' \
    "$fk/etc/passwd" || { echo "FAIL: guest-user passwd"; exit 1; }
grep -q '^video:x:39:hibiscus$' "$fk/etc/group" \
    || { echo "FAIL: guest-user video group"; exit 1; }
grep -q 'autologin hibiscus' \
    "$fk/etc/systemd/system/getty@.service.d/autologin.conf" \
    || { echo "FAIL: guest-user getty drop-in"; exit 1; }
bash vm/guest-user.sh "$fk"
[ "$(grep -c '^hibiscus:' "$fk/etc/passwd")" = 1 ] \
    || { echo "FAIL: guest-user not idempotent"; exit 1; }
echo "guest-user passed"

if make -s >/dev/null 2>&1 && [ -x out/vmd ]; then
    fake="$OUT/fakeguest"
    mkdir -p "$fake"
    printf 'GUEST_ARCH=%s\nGUEST_KERNEL=vmlinuz\nGUEST_ROOTFS=rootfs.img\n' \
        "$(uname -m)" > "$fake/guest.env"
    echo k > "$fake/vmlinuz"
    echo r > "$fake/rootfs.img"
    cp out/vmd "$OUT/vmd-embed"
    tar -cJf "$OUT/fakeguest.tar.xz" -C "$fake" .
    bash vm/embed-guest.sh "$OUT/vmd-embed" "$OUT/fakeguest.tar.xz"
    "$OUT/vmd-embed" -img "$OUT/empty-img" -guestinfo \
        | grep -q "embedded: guest bundle" \
        || { echo "FAIL: embedded bundle not found" >&2; exit 1; }
    echo "embed round trip passed"
else
    echo "run.sh: skipping embed test (vmd not buildable here)" >&2
fi

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

#!/usr/bin/env bash
# Fetch + assemble everything vmd needs to boot a Hibiscus vmd image in
# QEMU on a generic PC: an aarch64 virtio-capable kernel, the matching
# vendor partition, a bootstrap ramdisk, and a scratch userdata image.
#
# usage: vmd/bootenv/make-bootenv.sh [-o OUTDIR] [-h]
#   OUTDIR defaults to $HIBISCUS_ROOT/out (or ./out). Drops in:
#     kernel-gki      raw aarch64 kernel Image (GKI 5.4, virtio via modules)
#     vendor.img      API 29 emulator vendor partition (ext4, raw)
#     vmd-boot.img    cpio.gz bootstrap ramdisk (ramdisk/ sources + modules)
#     userdata.img    empty ext4 scratch disk (created once, kept across runs)
#
# Downloads are pinned by sha256 so a corrupted mirror fails loudly.
#
# The GKI kernel has no built-in virtio or binder chardevs: the ramdisk
# init loads the virtual-device module set in dependency order, and
# init.rc mounts binderfs so /dev/binder et al exist for Android Q.

set -euo pipefail
cd "$(dirname "$0")"

SDK_ZIP=arm64-v8a-29_r08.zip
SDK_URL=https://dl.google.com/android/repository/sys-img/android/$SDK_ZIP
SDK_SHA=7fd1610e16a69ac2e202278e82077de134ce404fe099d872019f69f3a340acdd

BUSYBOX_APK=busybox-static-1.36.1-r31.apk
BUSYBOX_URL=https://dl-cdn.alpinelinux.org/alpine/v3.20/main/aarch64/$BUSYBOX_APK
BUSYBOX_SHA=1d8e7a7fc2ed69bdb8eb7be9d962489c21a0f75b72d8a325437c8a09d4cfac70

# GKI 5.4 prebuilt + virtual-device module set (gitiles +/, ?format=TEXT
# serves files base64-encoded; sha256 covers the *decoded* bytes so a
# mirror swap still fails the pin).
GKI_REPO=https://android.googlesource.com/kernel/prebuilts/5.4/arm64
GKI_REF=master
GKI_KERNEL=kernel-5.4
GKI_KERNEL_SHA=641cec0c33280e58f8a2db43f94974262855be9e6bbb9c5d3b7d7981613cabae

VD_REPO=https://android.googlesource.com/kernel/prebuilts/common-modules/virtual-device/5.4/arm64
VD_REF=master
# load order matters: transport+deps first, then consumers
VD_MODULES="virtio_mmio.ko virtio_pci.ko virtio_blk.ko failover.ko net_failover.ko virtio_net.ko virtio-gpu.ko virtio_input.ko virtio_console.ko virtio-rng.ko"
declare -A VD_SHA=(
  [virtio_mmio.ko]=5236d43936f539da49dde1c19da6e1e47fb25f44ad58c1fbbff9e5bbd2507963
  [virtio_pci.ko]=49c7e3ee88b4a990b250a3a52d2b05f12901bc2b83afe69411188f8c4cdf340b
  [virtio_blk.ko]=f8a8cdf3a58ed20c278d9721e89ed8d83c4d2614133737e59cd04c9a90ff93ec
  [failover.ko]=3fd39de79bba0b05ef118d6dae0a02f647da6c7f3c8fa78384f9d5dc86d14b7b
  [net_failover.ko]=ae4ac5e6f10569f749fadc9e2e4a64ecc967ba110bc0d10f441b9925b588859b
  [virtio_net.ko]=41f0b5544fb5fc172e9a8b8406092303f2c74334226f3ed99f73d216f1da7361
  [virtio-gpu.ko]=4171741557c77891546944ccc8738d53857489045a854afb28031454da60fe06
  [virtio_input.ko]=f45fe1aaf741639a48ea5e8b6101bee16c99e174e1cc40bd2873b0e679335bb3
  [virtio_console.ko]=08795f862bb4671c4654a3be095e50455553da12ac1c799fb9c47b7963906ddb
  [virtio-rng.ko]=28ca936a0097b9e1018c924530274a99feeb1217bf6e30adba6a87c92447dd44
)

OUT="${HIBISCUS_ROOT:-${PN2_ROOT:-$PWD/..}}/out"
while [ $# -gt 0 ]; do
    case "$1" in
        -o) OUT="$2"; shift 2 ;;
        -h|--help) sed -n '2,14p' "$0"; exit 0 ;;
        *) echo "unknown arg: $1" >&2; exit 2 ;;
    esac
done
mkdir -p "$OUT"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

fetch() { # fetch <url> <sha> <dest>
    if [ ! -f "$WORK/$3" ]; then
        echo "bootenv: fetch $3"
        curl -fL --retry 3 -o "$WORK/$3" "$1"
    fi
    echo "$2  $WORK/$3" | sha256sum -c - >/dev/null
}

# gitiles serves raw file bytes base64-wrapped at ?format=TEXT
fetch_b64() { # fetch_b64 <repo> <ref> <path> <sha>
    local dest; dest=$(basename "$3")
    if [ ! -f "$WORK/$dest" ]; then
        echo "bootenv: fetch $3"
        curl -fL --retry 3 "$1/+/$2/$3?format=TEXT" | base64 -d > "$WORK/$dest"
    fi
    echo "$4  $WORK/$dest" | sha256sum -c - >/dev/null
}

# --- kernel: GKI 5.4 arm64 prebuilt -----------------------------------
fetch_b64 "$GKI_REPO" "$GKI_REF" "$GKI_KERNEL" "$GKI_KERNEL_SHA"
cp "$WORK/$GKI_KERNEL" "$OUT/kernel-gki"

# --- vendor: API 29 arm64 emulator vendor partition -------------------
fetch "$SDK_URL" "$SDK_SHA" sdk.zip
unzip -o -q "$WORK/sdk.zip" -d "$WORK/sdk" arm64-v8a/vendor.img
# vendor.img is a GPT disk; the sole partition is the ext4 vendor fs.
# Its fstab.ranchu declares 'logical' dynamic partitions (super) which
# Android Q first-stage init then waits for - strip it so fs_mgr uses
# the ramdisk's fstab instead.
read -r START COUNT < <(fdisk -l "$WORK/sdk/arm64-v8a/vendor.img" \
    | awk '/vendor.img1/ {print $2, $4}')
dd if="$WORK/sdk/arm64-v8a/vendor.img" of="$WORK/vendor.raw" bs=512 \
    skip="$START" count="$COUNT" status=none
debugfs -w -R 'rm /etc/fstab.ranchu' "$WORK/vendor.raw" >/dev/null 2>&1 || true

# vmd graphics HALs: stock QEMU has no /dev/qemu_pipe so the ranchu
# gralloc/hwc can never present. The vendor's generic allocator@2.0 /
# mapper@2.0 / composer@2.1 passthrough services wrap legacy modules
# (gralloc.<prop>.so, hwcomposer.<prop>.so); the ramdisk init points the
# prop at 'vmd' and these freestanding modules provide it - ashmem
# buffers for gralloc, a dumb-buffer KMS scanout for hwcomposer.
bash ../hal/build-hal.sh "$WORK/hal"
for m in gralloc hwcomposer; do
    debugfs -w -R "write $WORK/hal/$m.vmd.so /lib64/hw/$m.vmd.so" "$WORK/vendor.raw" >/dev/null
    debugfs -w -R "sif /lib64/hw/$m.vmd.so mode 0100755" "$WORK/vendor.raw" >/dev/null
done
cp "$WORK/vendor.raw" "$OUT/vendor.img"

# --- bootstrap ramdisk ------------------------------------------------
# The vmd system image is a bare non-SAR ext4 partition: no /init of its
# own, nothing for the kernel to exec. A tiny busybox initramfs mounts
# it (probing each virtio-blk disk by content since raw drives carry no
# partition names), then execs the image's own /system/bin/init which
# runs normal Android first stage.
fetch "$BUSYBOX_URL" "$BUSYBOX_SHA" busybox.apk
mkdir -p "$WORK/bb" "$WORK/rd/modules"
tar xzf "$WORK/busybox.apk" -C "$WORK/bb" bin/busybox.static 2>/dev/null
cp -a ramdisk/. "$WORK/rd/"

# kernel modules for the virtual-device module set (GKI has none built in)
for m in $VD_MODULES; do
    fetch_b64 "$VD_REPO" "$VD_REF" "$m" "${VD_SHA[$m]}"
    cp "$WORK/$m" "$WORK/rd/modules/"
done

# mountpoints/devices the bootstrap init needs - empty dirs don't
# survive in git, so they are materialized here
for d in bin proc sys dev probe system vendor data acct apex \
    debug_ramdisk mnt storage sbin oem postinstall config cache \
    bugreports sdcard d metadata mnt/asec mnt/user; do
    mkdir -p "$WORK/rd/$d"
done
install -Dm755 "$WORK/bb/bin/busybox.static" "$WORK/rd/bin/busybox"
( cd "$WORK/rd" && find . | cpio -o -H newc --quiet | gzip -9 ) \
    > "$OUT/vmd-boot.img"

# --- scratch userdata -------------------------------------------------
if [ ! -f "$OUT/userdata.img" ]; then
    truncate -s 512M "$OUT/userdata.img"
    mkfs.ext4 -q "$OUT/userdata.img"
fi

echo "bootenv: wrote kernel-gki, vendor.img, vmd-boot.img, userdata.img to $OUT"
echo "bootenv: run 'vmd -img $OUT' - it picks these files up automatically"

#!/usr/bin/env bash
# Fetch + assemble everything vmd needs to boot a Hibiscus vmd image in
# QEMU on a generic PC: an aarch64 virtio-capable kernel, the matching
# vendor partition, a bootstrap ramdisk, and a scratch userdata image.
#
# usage: vmd/bootenv/make-bootenv.sh [-o OUTDIR] [-h]
#   OUTDIR defaults to $PN2_ROOT/out (or ./out). Drops in:
#     kernel-ranchu   raw aarch64 kernel Image (virtio built-in)
#     vendor.img      API 29 emulator vendor partition (ext4, raw)
#     vmd-boot.img    cpio.gz bootstrap ramdisk (ramdisk/ sources here)
#     userdata.img    empty ext4 scratch disk (created once, kept across runs)
#
# Downloads are pinned by sha256 so a corrupted mirror fails loudly.

set -euo pipefail
cd "$(dirname "$0")"

SDK_ZIP=arm64-v8a-29_r08.zip
SDK_URL=https://dl.google.com/android/repository/sys-img/android/$SDK_ZIP
SDK_SHA=7fd1610e16a69ac2e202278e82077de134ce404fe099d872019f69f3a340acdd

BUSYBOX_APK=busybox-static-1.36.1-r31.apk
BUSYBOX_URL=https://dl-cdn.alpinelinux.org/alpine/v3.20/main/aarch64/$BUSYBOX_APK
BUSYBOX_SHA=1d8e7a7fc2ed69bdb8eb7be9d962489c21a0f75b72d8a325437c8a09d4cfac70

OUT="${PN2_ROOT:-$PWD/..}/out"
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

# --- kernel + vendor: API 29 arm64 emulator system image -------------
fetch "$SDK_URL" "$SDK_SHA" sdk.zip
unzip -o -q "$WORK/sdk.zip" -d "$WORK/sdk" arm64-v8a/kernel-ranchu arm64-v8a/vendor.img
# kernel-ranchu ships gzip-compressed; qemu -kernel wants raw Image
gunzip -c "$WORK/sdk/arm64-v8a/kernel-ranchu" > "$OUT/kernel-ranchu"

# vendor.img is a GPT disk; the sole partition is the ext4 vendor fs.
# Its fstab.ranchu declares 'logical' dynamic partitions (super) which
# Android Q first-stage init then waits for - strip it so fs_mgr uses
# the ramdisk's fstab instead.
read -r START COUNT < <(fdisk -l "$WORK/sdk/arm64-v8a/vendor.img" \
    | awk '/vendor.img1/ {print $2, $4}')
dd if="$WORK/sdk/arm64-v8a/vendor.img" of="$WORK/vendor.raw" bs=512 \
    skip="$START" count="$COUNT" status=none
debugfs -w -R 'rm /etc/fstab.ranchu' "$WORK/vendor.raw" >/dev/null 2>&1 || true
cp "$WORK/vendor.raw" "$OUT/vendor.img"

# --- bootstrap ramdisk ------------------------------------------------
# The vmd system image is a bare non-SAR ext4 partition: no /init of its
# own, nothing for the kernel to exec. A tiny busybox initramfs mounts
# it (probing each virtio-blk disk by content since raw drives carry no
# partition names), then execs the image's own /system/bin/init which
# runs normal Android first stage.
fetch "$BUSYBOX_URL" "$BUSYBOX_SHA" busybox.apk
mkdir -p "$WORK/bb" "$WORK/rd"
tar xzf "$WORK/busybox.apk" -C "$WORK/bb" bin/busybox.static 2>/dev/null
cp -a ramdisk/. "$WORK/rd/"
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

echo "bootenv: wrote kernel-ranchu, vendor.img, vmd-boot.img, userdata.img to $OUT"
echo "bootenv: run 'vmd -img $OUT' - it picks these files up automatically"

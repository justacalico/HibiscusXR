#!/bin/bash
# Installs the built Turnip driver as the system Vulkan HAL module on the
# headset. Backs up the stock module the first time.
set -euo pipefail

cd "$(dirname "$0")"
OUT="$(pwd)/out"

[ -f "$OUT/libvulkan_freedreno.so" ] || { echo "run build.sh first"; exit 1; }

adb shell 'mount -o remount,rw /vendor 2>/dev/null; mount -o remount,rw / 2>/dev/null; true'

adb push "$OUT/libvulkan_freedreno.so" /data/local/tmp/
adb push "$OUT/libc++_shared.so" /data/local/tmp/

adb shell '
set -e
if [ ! -f /vendor/lib64/hw/vulkan.sdm845.so.orig ]; then
    cp /vendor/lib64/hw/vulkan.sdm845.so /vendor/lib64/hw/vulkan.sdm845.so.orig
fi
cp /data/local/tmp/libvulkan_freedreno.so /vendor/lib64/hw/vulkan.sdm845.so
cp /data/local/tmp/libc++_shared.so /vendor/lib64/libc++_shared.so
chmod 644 /vendor/lib64/hw/vulkan.sdm845.so /vendor/lib64/libc++_shared.so
chcon u:object_r:vendor_file:s0 /vendor/lib64/hw/vulkan.sdm845.so 2>/dev/null || true
chcon u:object_r:vendor_file:s0 /vendor/lib64/libc++_shared.so 2>/dev/null || true
echo installed
'

adb shell 'rm -f /data/local/tmp/libvulkan_freedreno.so /data/local/tmp/libc++_shared.so'
echo "Turnip HAL module installed. Stock module at vulkan.sdm845.so.orig."

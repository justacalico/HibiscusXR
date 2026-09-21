#!/bin/bash
# Restores the stock Qualcomm Vulkan HAL module.
set -euo pipefail

adb shell 'mount -o remount,rw /vendor 2>/dev/null; mount -o remount,rw / 2>/dev/null; true'
adb shell '
if [ -f /vendor/lib64/hw/vulkan.sdm845.so.orig ]; then
    cp /vendor/lib64/hw/vulkan.sdm845.so.orig /vendor/lib64/hw/vulkan.sdm845.so
    chmod 644 /vendor/lib64/hw/vulkan.sdm845.so
    echo restored
else
    echo "no backup found"
fi
'

#!/system/bin/sh
# Is Vulkan 1.0.3 a real driver limit, or just what a permission XML declares?
echo "=========== who declares android.hardware.vulkan.version ==========="
grep -rl "vulkan.version" /vendor/etc/permissions /system/etc/permissions 2>/dev/null
echo "--- contents ---"
for f in $(grep -rl "vulkan" /vendor/etc/permissions /system/etc/permissions 2>/dev/null); do
  echo "  == $f =="; cat "$f"
done

echo
echo "=========== available vulkan permission XMLs on the system ==========="
ls -l /vendor/etc/permissions/ /system/etc/permissions/ 2>/dev/null | grep -i vulkan

echo
echo "=========== does the loader see a 1.1-capable ICD? ==========="
ls -l /vendor/lib64/hw/vulkan.*.so /system/lib64/hw/vulkan.*.so 2>/dev/null
getprop ro.hardware.vulkan
echo "--- vendor gpu driver version ---"
getprop ro.vendor.gpu.available_frequencies 2>/dev/null | head -c 100; echo
strings -a /vendor/lib64/egl/libGLESv2_adreno.so 2>/dev/null | grep -oE "V@[0-9]+\.[0-9]+" | sort -u | head -3

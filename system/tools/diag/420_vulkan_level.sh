#!/system/bin/sh
# ASPO-neo2 claims the Neo 2 is Vulkan 1.0 only. That matters a lot for Monado's
# compositor, so check what our Android 10 GSI + the 2021 vendor blob actually report.
echo "=========== declared vulkan features ==========="
pm list features 2>/dev/null | grep -i vulkan
echo "  (version is a packed int: 4194304=1.0.0, 4198400=1.1.0, 4202496=1.2.0)"

echo
echo "=========== gles version for comparison ==========="
pm list features 2>/dev/null | grep -i opengles
dumpsys SurfaceFlinger 2>/dev/null | grep -iE "GLES:|OpenGL ES|Vulkan" | head -5

echo
echo "=========== what the driver blob itself advertises ==========="
ls -l /vendor/lib64/hw/vulkan.sdm845.so
strings -a /vendor/lib64/hw/vulkan.sdm845.so 2>/dev/null | grep -iE "VK_API_VERSION|VK_VERSION_1_|Adreno.*Vulkan|^1\.[0-9]\.[0-9]+$" | sort -u | head -20
echo "--- vulkan 1.1 core entry points present in the blob? ---"
for s in vkEnumerateInstanceVersion vkGetPhysicalDeviceFeatures2 vkBindBufferMemory2 \
         vkCreateDescriptorUpdateTemplate vkGetPhysicalDeviceProperties2; do
  if strings -a /vendor/lib64/hw/vulkan.sdm845.so 2>/dev/null | grep -qx "$s"; then
    echo "  present  $s"
  else
    echo "  ABSENT   $s"
  fi
done

echo
echo "=========== driver version string ==========="
strings -a /vendor/lib64/hw/vulkan.sdm845.so 2>/dev/null | grep -iE "Adreno|Qualcomm|0[45][0-9]{2}\.[0-9]" | sort -u | head -10

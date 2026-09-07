#!/system/bin/sh
echo "=== md5 of the 32-bit client and friends ==="
md5sum /system/lib/libpvrserviceclient.so \
       /system/lib64/libpvrserviceclient.so \
       /system/priv-app/CVService/lib/arm/libPvr_UnitySDKCV.so \
       /system/priv-app/CVService/lib/arm/libCVController.so 2>/dev/null
echo
echo "=== DT_NEEDED of the 32-bit client ==="
strings -a /system/lib/libpvrserviceclient.so 2>/dev/null | grep -E "^lib.*\.so$" | sort -u
echo
echo "=== are all of those present in /system/lib? ==="
for l in $(strings -a /system/lib/libpvrserviceclient.so 2>/dev/null | grep -E "^lib.*\.so$" | sort -u); do
  if [ -f "/system/lib/$l" ] || [ -f "/vendor/lib/$l" ] || [ -f "/apex/com.android.runtime/lib/$l" ]; then :; else echo "  MISSING: $l"; fi
done
echo "  (nothing above = closure complete)"
echo
echo "=== 32-bit libs the CV blob needs ==="
for l in $(strings -a /system/priv-app/CVService/lib/arm/libPvr_UnitySDKCV.so 2>/dev/null | grep -E "^lib.*\.so$" | sort -u); do
  if [ -f "/system/lib/$l" ] || [ -f "/vendor/lib/$l" ] || [ -f "/apex/com.android.runtime/lib/$l" ] || [ -f "/system/priv-app/CVService/lib/arm/$l" ]; then :; else echo "  MISSING: $l"; fi
done
echo "  (nothing above = closure complete)"

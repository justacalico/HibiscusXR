#!/system/bin/sh
# What actually pins the Android version now that Pico's 8.1-era stack is leaving?
echo "=========== VNDK snapshots our GSI carries ==========="
ls -d /system/lib64/vndk* /system/lib/vndk* 2>/dev/null
echo "  ro.vndk.version = $(getprop ro.vndk.version)"
echo "  ro.vndk.lite    = $(getprop ro.vndk.lite)"

echo
echo "=========== graphics HAL versions the vendor provides ==========="
echo "--- from the vendor manifest ---"
grep -A3 -iE "graphics.(mapper|allocator|composer)" /vendor/manifest.xml 2>/dev/null | grep -iE "name|version" | head -20
echo "--- actual hal services running ---"
ps -A -o NAME 2>/dev/null | grep -iE "allocator|composer|gralloc" | sort -u

echo
echo "=========== all HAL interface versions vendor declares ==========="
grep -oE "<name>android\.hardware\.[a-z.]+</name>|<version>[0-9.]+</version>" /vendor/manifest.xml 2>/dev/null | paste - - 2>/dev/null | head -40

echo
echo "=========== what qvrservice links against (the new pinning constraint) ==========="
echo "--- NEEDED entries ---"
strings -a /system/bin/qvrservice 2>/dev/null | grep -E "^lib[a-z0-9_+.-]+\.so$" | sort -u
echo "--- and the client libs ---"
for l in /system/lib64/libqvrservice_client.so /system/lib64/libqvrcamera_client.so; do
  echo "  == $l =="
  strings -a "$l" 2>/dev/null | grep -E "^lib[a-z0-9_+.-]+\.so$" | sort -u | tr '\n' ' '; echo
done

echo
echo "=========== kernel + api level facts a newer GSI would face ==========="
echo "  kernel   : $(uname -r)"
echo "  sdk      : $(getprop ro.build.version.sdk)"
echo "  first api: $(getprop ro.product.first_api_level)"
echo "  vendor api levels supported by /vendor:"
getprop ro.vendor.build.version.sdk
echo "  fscrypt  : $(getprop ro.crypto.type) $(getprop ro.crypto.volume.filenames_mode)"

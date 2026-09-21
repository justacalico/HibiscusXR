#!/system/bin/sh
# libcdsprpc.so is missing and it takes the whole tracking stack down with it.
# Find every copy on this device and anything related (adsprpc is its sibling).
echo "=== libcdsprpc / libadsprpc anywhere ==="
for d in /system/lib /system/lib64 /vendor/lib /vendor/lib64 /system/vendor/lib /system/vendor/lib64 /odm/lib /odm/lib64; do
  [ -d "$d" ] || continue
  ls -l "$d" 2>/dev/null | grep -iE 'cdsprpc|adsprpc|libsdsprpc' | sed "s|^|  $d/ |"
done
echo
echo "=== dsp rpc daemons ==="
ps -A 2>/dev/null | grep -iE 'rpcd|dsp'
ls -l /system/bin/*rpcd* /vendor/bin/*rpcd* 2>/dev/null
echo
echo "=== what the qvr cdsp stub actually needs ==="
if [ -f /system/lib/libqvr_cdsp_driver_stub.so ]; then
  strings -a /system/lib/libqvr_cdsp_driver_stub.so 2>/dev/null | grep -iE '\.so$' | sort -u | head
fi

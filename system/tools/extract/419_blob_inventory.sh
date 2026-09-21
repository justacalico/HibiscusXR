#!/system/bin/sh
# For an XDA-style "flash system only" image: /vendor and /persist stay on the
# user's device, so anything living there is free. The problem set is whatever
# we need that lives on /system, because our flash replaces it.
echo "=========== does qvrservice exist on /vendor, or only /system? ==========="
for p in /system/bin/qvrservice /vendor/bin/qvrservice /system/bin/qvrd /vendor/bin/qvrd; do
  [ -e "$p" ] && echo "  PRESENT  $p  ($(wc -c < $p) bytes)" || echo "  absent   $p"
done

echo
echo "=========== QVR libs: vendor copy present (= free) ? ==========="
for l in libqvrservice_client.so libqvrcamera_client.so; do
  for d in /vendor/lib64 /vendor/lib /system/lib64 /system/lib; do
    [ -e "$d/$l" ] && echo "  $d/$l  $(wc -c < $d/$l)"
  done
done

echo
echo "=========== DSP transport + skels: which partition ==========="
for l in libcdsprpc.so libadsprpc.so libmdsprpc.so; do
  for d in /vendor/lib64 /system/lib64; do
    [ -e "$d/$l" ] && echo "  $d/$l"
  done
done
echo "--- adsprpcd / cdsprpcd ---"
for p in /vendor/bin/adsprpcd /system/bin/adsprpcd /vendor/bin/cdsprpcd /system/bin/cdsprpcd; do
  [ -e "$p" ] && echo "  PRESENT  $p"
done
echo "--- rfsa/adsp skels ---"
echo "  vendor: $(ls /vendor/lib/rfsa/adsp 2>/dev/null | wc -l) files"
echo "  system: $(ls /system/lib/rfsa/adsp 2>/dev/null | wc -l) files"

echo
echo "=========== fancontrol: which partition ==========="
for p in /system/bin/fancontrol /vendor/bin/fancontrol; do
  [ -e "$p" ] && echo "  PRESENT  $p  ($(wc -c < $p) bytes)"
done
echo "--- is the sysfs governor interface writable (open replacement viable)? ---"
ls -l /sys/class/thermal/cooling_device*/cur_state 2>/dev/null | head -3
cat /sys/class/thermal/cooling_device0/type 2>/dev/null

echo
echo "=========== config files: which partition ==========="
for f in /vendor/etc/qvr/qvrservice_config_default.txt /vendor/etc/qvr/6dof_config.xml \
         /system/etc/pvr/pxr_config.txt /persist/pvr/camera/device_calibration.xml \
         /persist/pvr/lens/axisOffset.txt /persist/calibration/Gyrooffset.txt \
         /persist/calibration/Bosh/IMUParams.txt; do
  [ -e "$f" ] && echo "  PRESENT  $f" || echo "  MISSING  $f"
done

echo
echo "=========== graphics/display: vendor-side? ==========="
ls -l /vendor/lib64/hw/vulkan.sdm845.so /vendor/lib64/egl/libGLESv2_adreno.so 2>/dev/null | head
ls /vendor/lib64/hw/ 2>/dev/null | grep -iE "gralloc|hwcomposer|memtrack" | head

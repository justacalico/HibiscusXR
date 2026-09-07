set -e
mount -o rw,remount /system
for l in libvirtualinputclient.so libairclient.so libSafetyArea.so libImageGrid.so; do
  for d in lib64 lib; do
    s="/data/local/tmp/pvr_${d}_${l}"
    if [ -f "$s" ]; then
      cp "$s" "/system/$d/$l"
      chmod 644 "/system/$d/$l"; chown root:root "/system/$d/$l"
      chcon u:object_r:system_file:s0 "/system/$d/$l" 2>/dev/null || true
    fi
  done
done
cp /system/etc/public.libraries.txt /system/etc/public.libraries.txt.gsi 2>/dev/null || true
cp /data/local/tmp/public.libraries.txt /system/etc/public.libraries.txt
chmod 644 /system/etc/public.libraries.txt
sync
echo "--- installed ---"
ls -l /system/lib64/libvirtualinputclient.so /system/lib64/libairclient.so /system/lib64/libSafetyArea.so /system/lib64/libImageGrid.so
echo "--- public.libraries.txt tail ---"
tail -8 /system/etc/public.libraries.txt

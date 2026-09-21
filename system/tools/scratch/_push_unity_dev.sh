#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp -f /data/local/tmp/unitylibs/lib64/*.so /system/lib64/ 2>/dev/null
cp -f /data/local/tmp/unitylibs/lib/*.so   /system/lib/   2>/dev/null
for f in /system/lib64/libPvr_*.so /system/lib/libPvr_*.so \
         /system/lib64/lib2dToVr.so /system/lib/lib2dToVr.so \
         /system/lib64/libvraudio.so /system/lib/libvraudio.so \
         /system/lib64/libpicologkit.so; do
  [ -f "$f" ] || continue
  chown root:root "$f"
  chmod 644 "$f"
done
sync
mount -o ro,remount /system
echo "--- installed ---"
ls -l /system/lib64/libPvr_UnitySDK.so /system/lib/libPvr_UnitySDK.so
echo "lib64 Pvr libs: $(ls /system/lib64/libPvr_*.so 2>/dev/null | wc -l)"
echo "lib   Pvr libs: $(ls /system/lib/libPvr_*.so 2>/dev/null | wc -l)"
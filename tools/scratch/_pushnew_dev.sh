#!/system/bin/sh
exec 2>&1
mount -o rw,remount /system
cp -f /data/local/tmp/newlibs/lib64/*.so /system/lib64/ 2>/dev/null
cp -f /data/local/tmp/newlibs/lib/*.so   /system/lib/   2>/dev/null
for f in /data/local/tmp/newlibs/lib64/*.so; do
  b=$(basename "$f"); [ -f /system/lib64/$b ] && chown root:root /system/lib64/$b && chmod 644 /system/lib64/$b
done
for f in /data/local/tmp/newlibs/lib/*.so; do
  b=$(basename "$f"); [ -f /system/lib/$b ] && chown root:root /system/lib/$b && chmod 644 /system/lib/$b
done
sync
mount -o ro,remount /system
ls -l /system/lib64/lib6DofReset.so /system/lib/lib6DofReset.so
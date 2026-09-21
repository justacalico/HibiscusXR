#!/system/bin/sh
# Put the extracted Pico VR runtime onto the GSI's /system and try to start it.
S=/data/local/tmp/pvr_stack

echo "=== what are we dealing with (ELF class) ==="
for b in pvrservice qvrservice pvr_compute; do
  printf '%-14s ' "$b"
  head -c 5 "$S/bin/$b" | od -An -tx1 | tr -d ' \n'
  echo "   (7f454c4602 = 64-bit, 7f454c4601 = 32-bit)"
done
echo
echo "--- /bin/vr is a script? ---"
cat "$S/bin/vr"
echo

mount -o rw,remount /system

echo "=== installing ==="
cp -a "$S/lib64/." /system/lib64/ && echo "  lib64 ok"
cp -a "$S/lib/."   /system/lib/   && echo "  lib   ok"
cp -a "$S/bin/."   /system/bin/   && echo "  bin   ok"
cp -a "$S/framework/." /system/framework/ && echo "  framework ok"
mkdir -p /system/etc/pvr
cp -a "$S/etc/pvr/." /system/etc/pvr/ && echo "  etc/pvr ok"
cp "$S/etc/init/pvrservice.rc" /system/etc/init/pvrservice.rc && echo "  init rc ok"

chmod 755 /system/bin/pvrservice /system/bin/qvrservice /system/bin/pvr_compute /system/bin/vr
chmod 644 /system/etc/init/pvrservice.rc
chown root:root /system/etc/init/pvrservice.rc
find /system/etc/pvr -type f -exec chmod 644 {} \;
find /system/lib64 -name '*pxr*' -o -name '*pvr*' -o -name '*qvr*' | xargs -r chmod 644
sync
mount -o ro,remount /system

echo
echo "=== the dangling persist symlink should resolve now ==="
ls -l /persist/pvr/psmvrapi_config.txt
cat /persist/pvr/psmvrapi_config.txt 2>&1 | head -3
echo
echo "=== does pvrservice's linker actually resolve? ==="
LD_LIBRARY_PATH=/system/lib64 /system/bin/pvrservice --help 2>&1 | head -20
echo "rc=$?"
echo DONE

set -e
TGT=/system/apex/com.android.runtime.release/lib64/libart.so
mount -o rw,remount /system
if [ ! -f "$TGT.orig" ]; then cp -a "$TGT" "$TGT.orig"; echo "backup created"; else echo "backup already exists"; fi
cat /data/local/tmp/libart-patched.so > "$TGT"
chmod 644 "$TGT"; chown root:root "$TGT"
chcon u:object_r:system_file:s0 "$TGT" 2>/dev/null || true
sync
echo "--- sizes ---"
ls -l "$TGT" "$TGT.orig"
echo "--- md5 (target vs what we pushed) ---"
md5sum "$TGT" /data/local/tmp/libart-patched.so
mount -o ro,remount /system

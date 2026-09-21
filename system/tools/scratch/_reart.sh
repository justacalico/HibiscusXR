T=/system/apex/com.android.runtime.release/lib64/libart.so
mount -o rw,remount /system
[ -f "$T.orig" ] || cp -a "$T" "$T.orig"
cp /data/local/tmp/libart-patched.so "$T.new"
chmod 644 "$T.new"; chown root:root "$T.new"
chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null
mv -f "$T.new" "$T"
sync
md5sum "$T" /data/local/tmp/libart-patched.so

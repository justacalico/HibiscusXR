T=/system/apex/com.android.runtime.release/lib64/libart.so
mount -o rw,remount /system
if [ -f "$T.orig" ]; then cp -a "$T.orig" "$T"; echo "reverted to stock ART"; else echo "NO BACKUP"; fi
sync
md5sum "$T" "$T.orig"

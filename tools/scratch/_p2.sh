T=/system/priv-app/VRShell2/lib/arm64/libPvr_UnitySDK.so
mount -o rw,remount /system
cp /data/local/tmp/libPvr_p2.so "$T.new"
chmod 644 "$T.new"; chown root:root "$T.new"
chcon u:object_r:system_file:s0 "$T.new" 2>/dev/null
mv -f "$T.new" "$T"
sync
md5sum "$T"

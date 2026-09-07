mount -o rw,remount /system
mkdir -p /system/lib64/pvr_air
for f in /data/local/tmp/air64_*.so; do b=$(basename $f); b=${b#air64_}; cp "$f" "/system/lib64/pvr_air/$b"; done
for f in /data/local/tmp/sh64_*.so;  do b=$(basename $f); b=${b#sh64_};  cp "$f" "/system/lib64/$b"; done
chmod 755 /system/lib64/pvr_air
chmod 644 /system/lib64/pvr_air/*.so /system/lib64/libvirtualinput.so
chown -R root:root /system/lib64/pvr_air
chcon -R u:object_r:system_lib_file:s0 /system/lib64/pvr_air 2>/dev/null || chcon -R u:object_r:system_file:s0 /system/lib64/pvr_air 2>/dev/null
cp /data/local/tmp/bin_airservice /system/bin/airservice
cp /data/local/tmp/bin_virtual_input /system/bin/virtual_input
chmod 755 /system/bin/airservice /system/bin/virtual_input
chown root:root /system/bin/airservice /system/bin/virtual_input
chcon u:object_r:system_file:s0 /system/bin/airservice /system/bin/virtual_input 2>/dev/null
cp /data/local/tmp/pn2-airservice.rc /system/etc/init/pn2-airservice.rc
chmod 644 /system/etc/init/pn2-airservice.rc
sync
echo "--- installed ---"
ls -l /system/bin/airservice /system/bin/virtual_input
ls /system/lib64/pvr_air/
ls -l /system/lib64/libvirtualinput.so
rm -f /data/local/tmp/air64_*.so /data/local/tmp/sh64_*.so /data/local/tmp/bin_*

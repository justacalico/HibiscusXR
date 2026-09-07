mount -o rw,remount /system
cp /data/local/tmp/pl.txt /system/etc/public.libraries.txt
chmod 644 /system/etc/public.libraries.txt
chown root:root /system/etc/public.libraries.txt
sync
echo "--- entries: $(grep -c "\.so$" /system/etc/public.libraries.txt) ---"
tail -5 /system/etc/public.libraries.txt

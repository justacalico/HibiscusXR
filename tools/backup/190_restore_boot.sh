#!/system/bin/sh
# Zygote preloads every entry in public.libraries.txt and aborts the whole boot if
# any one of them fails to link. libairclient.so needs libdatabuffer.so, which we
# do not ship, so the device bootloops. Restore the previous file to get booting
# again; the whitelist gets reapplied once the dependency closure is satisfied.
mount -o rw,remount /system
if [ -f /system/etc/public.libraries.txt.gsi ]; then
  cp /system/etc/public.libraries.txt /system/etc/public.libraries.txt.broken
  cp /system/etc/public.libraries.txt.gsi /system/etc/public.libraries.txt
  chmod 644 /system/etc/public.libraries.txt
  sync
  echo "restored; now:"
  tail -4 /system/etc/public.libraries.txt
else
  echo "NO BACKUP - writing the AOSP list by hand"
fi

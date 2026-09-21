#!/system/bin/sh
echo "=== /vendor/manifest.xml as seen at runtime ==="
echo "size      : $(stat -c%s /vendor/manifest.xml)   (22335 = patched view, 22604 = raw vendor)"
echo "boot refs : $(grep -c 'hardware\.boot' /vendor/manifest.xml)   (0 = overlay active)"
echo
echo "=== bind mount in /proc/mounts ==="
grep -i manifest /proc/mounts || echo "(no manifest line)"
echo
echo "=== the underlying vendor partition file, unmounted view ==="
umount /vendor/manifest.xml 2>/dev/null && {
  echo "after unmount size      : $(stat -c%s /vendor/manifest.xml)"
  echo "after unmount boot refs : $(grep -c 'hardware\.boot' /vendor/manifest.xml)"
  mount none /system/etc/pn2/vendor_manifest.xml /vendor/manifest.xml bind
  echo "remounted overlay, size now $(stat -c%s /vendor/manifest.xml)"
} || echo "(could not unmount to peek; skipping)"
echo
echo "=== vold healthy? ==="
echo "pid $(pidof vold)  svc $(getprop init.svc.vold)"
echo "boot_completed=$(getprop sys.boot_completed) uptime=$(cut -d. -f1 /proc/uptime)s"
echo DONE

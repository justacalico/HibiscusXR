#!/system/bin/sh
echo "=== why init refused the qvrd domain ==="
dmesg 2>/dev/null | grep -iE 'init.*(qvrd|pn2_qvrd|seclabel|SELinux)' | tail -12
echo
echo "=== reverting to the domain that worked ==="
mount -o rw,remount /system
cp /system/etc/init/pn2-qvrd.rc.shell /system/etc/init/pn2-qvrd.rc
chmod 644 /system/etc/init/pn2-qvrd.rc
sync
grep seclabel /system/etc/init/pn2-qvrd.rc
echo
echo "(needs a reboot for init to re-read it)"

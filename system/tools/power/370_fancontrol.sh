#!/system/bin/sh
# Stock runs a native "fancontrol" daemon; we run nothing. We share the vendor
# partition, so the binary is probably already here and simply never started -
# same as qvrservice was.
echo "=== is the binary present? ==="
ls -lZ /vendor/bin/fancontrol /system/bin/fancontrol /vendor/bin/hw/fancontrol 2>/dev/null || echo "  not in the usual places"
find /system /vendor -name "fancontrol*" -o -name "*thermalserviced*" 2>/dev/null | head
echo
echo "=== which init rc defines it ==="
grep -rl "fancontrol" /vendor/etc/init/ /system/etc/init/ /init*.rc 2>/dev/null | head
echo "  --- the service block ---"
grep -rA8 "^service fancontrol" /vendor/etc/init/ /system/etc/init/ /init*.rc 2>/dev/null | head -20
echo
echo "=== why is it not running here ==="
getprop init.svc.fancontrol
echo "  (empty = init never saw the service)"
echo
echo "=== what does it link against ==="
strings -a /vendor/bin/fancontrol 2>/dev/null | grep -E "^lib.*\.so$" | sort -u
echo
echo "=== and what does it poke ==="
strings -a /vendor/bin/fancontrol 2>/dev/null | grep -E "^/(sys|dev|data)" | sort -u | head -20
echo
echo "=== thermalserviced ==="
ls -l /system/bin/thermalserviced /vendor/bin/thermalserviced 2>/dev/null
getprop init.svc.thermalserviced

#!/system/bin/sh
# Does /system/bin/modprobe actually need /etc/modprobe.d and /etc/modprobe.conf?
# I created them earlier on a wrong theory. If they are load-bearing they must go
# into the shipped overlay; if not they are dead weight and should be deleted.
# Modules are already loaded, so this is a no-op either way and audio is safe.
mount -o rw,remount /system
mv /system/etc/modprobe.d   /system/etc/.modprobe.d.off   2>/dev/null
mv /system/etc/modprobe.conf /system/etc/.modprobe.conf.off 2>/dev/null
sync

echo "=== with modprobe.d/conf ABSENT ==="
/system/bin/modprobe -a -d /vendor/lib/modules pinctrl-wcd wcd-dsp-glink snd-soc-wcd-spi snd-soc-sdm845
echo "rc=$?"

mv /system/etc/.modprobe.d.off   /system/etc/modprobe.d    2>/dev/null
mv /system/etc/.modprobe.conf.off /system/etc/modprobe.conf 2>/dev/null
sync

echo
echo "=== with them PRESENT again ==="
/system/bin/modprobe -a -d /vendor/lib/modules pinctrl-wcd wcd-dsp-glink snd-soc-wcd-spi snd-soc-sdm845
echo "rc=$?"

mount -o ro,remount /system
echo
ls -ld /system/etc/modprobe.d /system/etc/modprobe.conf
echo DONE

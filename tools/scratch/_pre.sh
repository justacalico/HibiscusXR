echo "=== will wireless adb come back after reboot? ==="
echo "  persist.pn2.adbwifi   = $(getprop persist.pn2.adbwifi)"
echo "  service.adb.tcp.port  = $(getprop service.adb.tcp.port)"
echo "  persist.adb.tcp.port  = $(getprop persist.adb.tcp.port)"
ls -l /system/etc/init/pn2-adbwifi.rc 2>/dev/null || echo "  pn2-adbwifi.rc MISSING"
echo
echo "=== is the patched lib really in place? ==="
md5sum /system/lib/libpvrserviceclient.so
echo "  (patched = ad7b078f28b56049fc8dd72d5b4eed1a)"
echo
echo "=== is anything still mapping the deleted inode? ==="
for p in $(pidof com.picovr.picovrlib.cvcontroller:RemoteService) $(pidof com.pvr.vrshell); do
  grep -c "libpvrserviceclient.so (deleted)" /proc/$p/maps 2>/dev/null | sed "s/^/  pid $p deleted-maps: /"
done

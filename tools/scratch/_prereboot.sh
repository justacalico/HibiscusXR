# make sure adb-over-wifi survives the reboot; persist.adb.tcp.port is the
# reliable one, service.adb.tcp.port is set late by the rc on boot_completed
setprop persist.adb.tcp.port 5555
setprop persist.pn2.adbwifi 1
pm enable com.picovr.picovrlib.cvcontroller >/dev/null 2>&1
echo "  persist.adb.tcp.port = $(getprop persist.adb.tcp.port)"
echo "  cvcontroller enabled = $(pm list packages -e | grep -c cvcontroller)"
echo "  fancontrol rc        = $(ls /system/etc/init/pn2-fanservice.rc 2>/dev/null)"
echo "  patched lib md5      = $(md5sum /system/lib/libpvrserviceclient.so | cut -d' ' -f1)"
sync

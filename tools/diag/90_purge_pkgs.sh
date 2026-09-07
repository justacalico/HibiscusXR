#!/system/bin/sh
# Deleting the APKs from /system did NOT unregister the packages - that state is
# in /data/system/packages.xml. com.pvr.pxrnotification is persistent, so
# ActivityManager kept starting a package whose code no longer exists, which is
# what kept rebooting the device.
exec 2>&1
for p in com.pvr.pxrnotification com.pvr.vrshell com.pvr.vrdisplay \
         com.pvr.configuration com.pvr.shortcut com.pvr.verify com.pvr.adapter \
         com.pico.provider.settings com.picovr.picovrlib.cvcontroller \
         com.pico.picotosvr com.picovr.initserver com.picovr.vrusercenter; do
  r=$(pm uninstall "$p" 2>&1)
  case "$r" in
    Success*) echo "  uninstalled  $p" ;;
    *)        r2=$(pm uninstall --user 0 "$p" 2>&1)
              echo "  $p -> $r / user0: $r2" ;;
  esac
done
echo
echo "=== still registered? ==="
pm list packages 2>/dev/null | grep -iE 'pvr|pico' || echo "  none - clean"
echo DONE

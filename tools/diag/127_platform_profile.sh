#!/system/bin/sh
# Did fixing ro.build.product change which platform profile pvrservice loads?
# Restart it and read the line directly.
exec 2>&1
logcat -c
setprop sys.pvr.vrservice.state 2
sleep 6
echo "pvrservice pid: $(pidof pvrservice)"
echo
echo "=== platform profile ==="
logcat -d | grep -i 'Platform config file' | tail -3
echo
echo "=== identity as pvrservice sees it ==="
for k in ro.build.product ro.product.model ro.product.device ro.picovr.product.name ro.pvr.hmd.type; do
  echo "  $k = $(getprop $k)"
done
echo
echo "=== which platform inis exist ==="
ls /system/etc/pvr/ | grep -i platform
echo DONE

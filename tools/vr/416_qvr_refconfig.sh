#!/system/bin/sh
echo "=========== do Pico's redirected config targets even exist? ==========="
for f in /data/misc/user/pxrconfig/config_default.txt /persist/vendorhw/pxr/qvrservice_config.txt; do
  if [ -e "$f" ]; then echo "  PRESENT  $f  ($(wc -c < $f) bytes)"; else echo "  MISSING  $f"; fi
done

echo
echo "=========== Qualcomm's stock 6dof tracker config ==========="
cat /vendor/etc/qvr/6dof_config.xml 2>/dev/null

echo
echo "=========== qvrservice binary: what is it ==========="
ls -l /system/bin/qvrservice /vendor/bin/qvrservice 2>/dev/null
head -c 16 /system/bin/qvrservice 2>/dev/null | od -An -c | head -1

echo
echo "=========== qvrservice_config_default.txt (Qualcomm reference) ==========="
grep -vE "^\s*#|^\s*$" /vendor/etc/qvr/qvrservice_config_default.txt 2>/dev/null | head -50

echo
echo "=========== svrapi_config_default.txt (Qualcomm reference optics/render) ==========="
grep -vE "^\s*#|^\s*$" /vendor/etc/qvr/svrapi_config_default.txt 2>/dev/null | head -40

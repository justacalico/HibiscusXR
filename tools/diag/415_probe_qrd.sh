#!/system/bin/sh
echo "=========== board identity ==========="
for p in ro.boot.hardware ro.boot.hardware.sku ro.boot.product.hardware.id \
         ro.product.board ro.board.platform ro.bootloader ro.revision \
         ro.vendor.product.device ro.vendor.product.model ro.hardware.keystore \
         ro.boot.hwc ro.boot.hwlevel ro.soc.model; do
  v=$(getprop $p); [ -n "$v" ] && echo "  $p = $v"
done
echo "--- dtb / board name from kernel ---"
cat /proc/device-tree/model 2>/dev/null; echo
cat /proc/device-tree/qcom,board-id 2>/dev/null | od -An -tx1 | head -2
cat /proc/device-tree/compatible 2>/dev/null | tr '\0' ' '; echo

echo
echo "=========== QVR service config as shipped ==========="
ls -lR /vendor/etc/qvr* /system/etc/qvr* 2>/dev/null
echo "--- qvr props ---"
getprop | grep -iE "qvr|xr\.|vr\.|svr" | head -30

echo
echo "=========== QVR plugins present on this board ==========="
ls -l /vendor/lib64/libqvr*.so /system/lib64/libqvr*.so 2>/dev/null
echo "--- plugin dirs ---"
ls -l /vendor/lib64/qvr /vendor/lib/qvr /system/lib64/qvr 2>/dev/null

echo
echo "=========== what params qvrservice advertises ==========="
strings -a /system/bin/qvrservice 2>/dev/null | grep -E "^(tracker|render|camera|eye|plugin|sixdof|display)-[a-z0-9-]+$" | sort -u | head -60

echo
echo "=========== which DSP the tracker uses ==========="
strings -a /system/bin/qvrservice 2>/dev/null | grep -iE "cdsp|adsp|hvx|fastcv|slam|6dof" | sort -u | head -30

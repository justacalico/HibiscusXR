#!/system/bin/sh
# The /dsp partition and skel libraries are identical to stock, so FastRPC failing
# to get a remote handle is configuration, not missing files. Dump everything
# fastrpc/dsp-related for a diff.
echo "=== fastrpc / dsp / adsp properties ==="
getprop 2>/dev/null | grep -iE 'fastrpc|adsp|cdsp|sdsp|dsp\.|unsigned|rpcd|qvr|vendor\.qti\..*dsp' | sort
echo
echo "=== who can open the rpc nodes ==="
ls -lZ /dev/adsprpc-smd /dev/adsprpc-smd-secure /dev/cdsprpc-smd 2>/dev/null
echo
echo "=== qvrservice identity ==="
P=$(pidof qvrservice)
echo "  pid $P"
cat /proc/$P/status 2>/dev/null | grep -E '^(Uid|Gid|Groups)'
cat /proc/$P/attr/current 2>/dev/null | sed 's/^/  context: /'
echo
echo "=== stock qvrd rc for comparison ==="
awk '/^service qvrd/,/^$/' /vendor/etc/init/hw/init.target.rc

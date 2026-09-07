#!/system/bin/sh
echo "=== partitions ==="
ls /dev/block/bootdevice/by-name/ 2>/dev/null | tr '\n' ' '
echo
echo "=== any _a/_b slots? ==="
ls /dev/block/bootdevice/by-name/ 2>/dev/null | grep -cE '_a$|_b$'
echo "=== sizes of the big ones ==="
for p in system vendor userdata cache boot recovery; do
  d=$(readlink -f /dev/block/bootdevice/by-name/$p 2>/dev/null)
  [ -n "$d" ] && echo "  $p -> $d  $(blockdev --getsize64 $d 2>/dev/null)"
done

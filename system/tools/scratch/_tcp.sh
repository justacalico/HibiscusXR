mount -o rw,remount /system
# drop any earlier attempt, then append with a guaranteed leading newline
sed -i -E "/^persist\.adb\.tcp\.port=/d" /system/build.prop
[ -n "$(tail -c1 /system/build.prop)" ] && echo "" >> /system/build.prop
echo "persist.adb.tcp.port=5555" >> /system/build.prop
sync
setprop persist.adb.tcp.port 5555
setprop service.adb.tcp.port 5555
stop adbd; start adbd
echo "--- build.prop tail ---"
tail -3 /system/build.prop
echo "--- sanity: no glued lines ---"
grep -nE "^[a-z].*=.*[a-z]+\.[a-z].*=" /system/build.prop || echo "  clean"

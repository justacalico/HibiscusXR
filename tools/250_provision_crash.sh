#!/system/bin/sh
echo "=== full java stack for the provision crash ==="
logcat -d -b crash 2>/dev/null | grep -A30 'com.picovr.provision' | tail -40
echo
echo "=== our locale ==="
getprop persist.sys.locale
getprop ro.product.locale
settings get system system_locales
settings get secure  __system_locales 2>/dev/null
echo "current: $(getprop ro.build.version.release) / $(settings get system time_12_24)"

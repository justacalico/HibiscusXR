#!/system/bin/sh
echo "  serial      : $(getprop ro.serialno)"
echo "  model       : $(getprop ro.product.model)  /  $(getprop ro.pvr.product.name)"
echo "  android     : $(getprop ro.build.version.release) (sdk $(getprop ro.build.version.sdk))"
echo "  build       : $(getprop ro.build.display.id)"
echo "  fingerprint : $(getprop ro.build.fingerprint)"
echo "  vendor fp   : $(getprop ro.vendor.build.fingerprint)"
echo "  uptime      : $(awk '{printf "%d min", $1/60}' /proc/uptime)"
echo -n "  stack       : "
if pidof pvrservice > /dev/null 2>&1; then echo -n "pvrservice "; fi
if pidof com.pvr.vrshell > /dev/null 2>&1; then echo -n "vrshell "; fi
if pidof qvrservice > /dev/null 2>&1; then echo -n "qvrservice "; fi
if pidof fancontrol > /dev/null 2>&1; then echo -n "fancontrol "; fi
echo
echo -n "  verdict     : "
if [ "$(getprop ro.build.version.sdk)" = "29" ]; then echo "OUR GSI BUILD (Android 10) - safe to modify"; else echo "STOCK PUI (Android $(getprop ro.build.version.release)) - READ ONLY"; fi

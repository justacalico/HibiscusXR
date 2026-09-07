#!/system/bin/sh
# Run under /system/xbin/su. Gets the one thing we couldn't read as shell:
# vold's userspace backtrace, plus the ANR trace's vold section.
V=$(pidof vold)
echo "vold pid=$V uptime=$(cut -d. -f1 /proc/uptime)s"
echo

echo "############ debuggerd native backtrace ############"
debuggerd -b $V 2>&1 | head -80
echo

echo "############ latest ANR: vold section ############"
A=$(ls -1t /data/anr/anr_* 2>/dev/null | head -1)
echo "file: $A"
sed -n "/Cmd line: \/system\/bin\/vold/,/^----- end/p" "$A" 2>/dev/null | head -70
echo

echo "############ ANR: system_server android.fg + main ############"
sed -n '/"main" prio/,/^$/p' "$A" 2>/dev/null | head -45
echo "-----"
sed -n '/"android.fg" prio/,/^$/p' "$A" 2>/dev/null | head -35
echo DONE

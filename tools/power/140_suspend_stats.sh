#!/system/bin/sh
# Has this device EVER completed a kernel suspend? If stock's success count is 0
# after a long uptime, Pico never suspends at all - pvrservice.platform holds a
# kernel wakelock permanently - and the port's crash is us exercising a path the
# vendor never uses.
exec 2>&1
echo "uptime: $(cut -d. -f1 /proc/uptime)s"
echo
echo "##### suspend_stats #####"
for f in /sys/power/suspend_stats/success /sys/power/suspend_stats/fail \
         /sys/power/suspend_stats/last_failed_dev /sys/power/suspend_stats/last_failed_step; do
  [ -f "$f" ] && echo "  $(basename $f): $(cat $f 2>/dev/null)"
done
echo
echo "##### full suspend_stats #####"
for f in /sys/power/suspend_stats/*; do
  [ -f "$f" ] && printf '  %-22s %s\n' "$(basename $f)" "$(cat $f 2>/dev/null)"
done
echo
echo "##### kernel wakelocks #####"
cat /sys/power/wake_lock 2>/dev/null
echo
echo "##### pvr sleep policy props #####"
getprop | grep -iE 'pvr.*(sleep|screenoff|static)'
echo DONE

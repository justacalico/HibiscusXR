#!/system/bin/sh
# Runs on either device. The port dies entering suspend with no log at all,
# which means the hang is below the framework. Both units share the same kernel
# and vendor, so the difference has to be in how userspace drives suspend:
# Android 8.1 had PowerManager poke /sys/power/state directly, Android 10 routes
# it through the ISystemSuspend HAL, and vendor daemons that take kernel
# wakelocks the old way can end up fighting it.
exec 2>&1
echo "##### android release #####"
getprop ro.build.version.release
echo
echo "##### system_suspend HAL present? (Q only) #####"
getprop init.svc.system_suspend
ps -A -o PID,NAME 2>/dev/null | grep -i suspend
lshal 2>/dev/null | grep -i suspend
echo
echo "##### kernel wakelock interface #####"
ls -l /sys/power/wake_lock /sys/power/wake_unlock /sys/power/state 2>&1
echo "state: $(cat /sys/power/state 2>/dev/null)"
echo
echo "##### currently held kernel wakelocks #####"
cat /sys/power/wake_lock 2>/dev/null
echo
echo "##### wakeup_count / autosleep #####"
echo "wakeup_count: $(cat /sys/power/wakeup_count 2>/dev/null)"
echo "autosleep   : $(cat /sys/power/autosleep 2>/dev/null)"
echo
echo "##### active wakeup sources (top 12 by active_since) #####"
if [ -f /d/wakeup_sources ]; then
  head -1 /d/wakeup_sources
  awk 'NR>1 && $6>0 {print}' /d/wakeup_sources 2>/dev/null | head -12
elif [ -f /sys/kernel/debug/wakeup_sources ]; then
  head -1 /sys/kernel/debug/wakeup_sources
  grep -v '	0	0	0	0	0	0	0	0	0' /sys/kernel/debug/wakeup_sources 2>/dev/null | head -12
else
  echo "  (no wakeup_sources node)"
fi
echo
echo "##### framework wakelocks #####"
dumpsys power 2>/dev/null | grep -A12 'Wake Locks:' | head -14
echo
echo "##### suspend-related props #####"
getprop | grep -iE 'suspend|autosleep|sleep' | head -12
echo DONE

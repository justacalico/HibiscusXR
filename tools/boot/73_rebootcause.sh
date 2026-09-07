#!/system/bin/sh
echo "=== why did it reboot ==="
echo "bootreason      : $(getprop ro.boot.bootreason)"
echo "last_reboot     : $(getprop sys.boot.reason 2>/dev/null)"
echo "boot_completed  : $(getprop sys.boot_completed)"
echo "uptime          : $(cut -d. -f1 /proc/uptime)s"
echo

echo "=== pstore: did the kernel leave a death note? ==="
ls -la /sys/fs/pstore/ 2>&1
for f in /sys/fs/pstore/*; do
  [ -f "$f" ] || continue
  echo "--- $f ---"
  tail -40 "$f"
done
echo

echo "=== pvrservice state ==="
echo "init.svc.pvrservice = $(getprop init.svc.pvrservice)"
echo "pid = $(pidof pvrservice)"
echo "does the rc have LD_PRELOAD?"
grep -i preload /system/etc/init/pvrservice.rc || echo "  NO - this build will crash in PVR::receiver"
echo

echo "=== has pvrservice been restarting in a loop? ==="
logcat -d 2>/dev/null | grep -cE 'Load libpvrmodule_orientationtracker.so success'
echo "  ^ number of pvrservice starts this boot"
echo

echo "=== fatals this boot ==="
logcat -d -b crash 2>/dev/null | grep -E 'Abort message|>>> ' | tail -12
echo
echo "=== anything that looks like a watchdog / panic ==="
logcat -d 2>/dev/null | grep -iE 'WATCHDOG|Rebooting|shutting down|kernel panic|SysRq' | tail -10
echo DONE

#!/system/bin/sh
# v2: never read /sys/power/wakeup_count - that read BLOCKS by design (it is the
# handshake system_suspend uses) and hung the previous run.
exec 2>&1
echo "release: $(getprop ro.build.version.release)"
echo "system_suspend svc: $(getprop init.svc.system_suspend)"
echo "power state opts  : $(cat /sys/power/state 2>/dev/null)"
echo
echo "##### kernel wakelocks held right now #####"
cat /sys/power/wake_lock 2>/dev/null
echo
echo "##### wakeup sources that are ACTIVE #####"
for f in /d/wakeup_sources /sys/kernel/debug/wakeup_sources; do
  [ -f "$f" ] || continue
  head -1 "$f"
  # column 6 is active_count; show rows where the source is currently active
  grep -vE '\s0\s+0\s+0\s+0\s+0\s+0\s+0\s+0\s+0\s*$' "$f" | head -14
  break
done
echo
echo "##### framework wake locks #####"
dumpsys power 2>/dev/null | sed -n '/Wake Locks:/,/^$/p' | head -12
echo
echo "##### who is pvrservice.platform #####"
echo "pvrservice pid: $(pidof pvrservice)"
echo DONE

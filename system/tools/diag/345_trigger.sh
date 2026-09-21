#!/system/bin/sh
# pvrservice system()s "am startservice ... action_type 45" from a binder thread.
# fork() takes every jemalloc arena lock; a concurrent binder thread in free()
# holds one, and the process wedges. Find out what makes it spawn so often.
echo "=== restart pvrservice to clear the deadlock ==="
logcat -c
stop pvrservice; sleep 2; start pvrservice
sleep 3
P=$(pidof pvrservice)
echo "fresh pid $P"
echo
i=0
while [ $i -lt 8 ]; do
  N=$(ps -A -o PPID 2>/dev/null | grep -c "^ *$P$")
  echo "  t+$((i*3))s  children=$N"
  i=$((i+1))
  sleep 3
done
echo
echo "=== what logged just before the spawns ==="
logcat -d | grep -iE "vrdisplay|action_type|startservice|NotificationClient|registerClient|removeNotification" | tail -20
echo
echo "=== is it still alive or wedged again? ==="
debuggerd -b $P 2>&1 | grep -cE "je_malloc_mutex|atfork"
echo "  (0 = healthy, >0 = malloc/fork contention again)"

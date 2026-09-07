#!/system/bin/sh
V=$(pidof vold)
echo "vold pid = $V"
echo "uptime: $(cut -d. -f1 /proc/uptime)s"
echo

echo "=== vold thread states (R=run S=sleep D=uninterruptible) ==="
for t in /proc/$V/task/*; do
  echo "[$(cat $t/comm)] state=$(awk '{print $3}' $t/stat) wchan=$(cat $t/wchan 2>/dev/null)"
done
echo

echo "=== vold kernel stacks ==="
for t in /proc/$V/task/*; do
  echo "--- $(cat $t/comm) ---"
  cat $t/stack 2>/dev/null | head -12
done
echo

echo "=== what has vold got open ==="
ls -l /proc/$V/fd 2>/dev/null | head -40
echo

echo "=== binder state for vold ==="
grep -A4 "proc $V\$" /sys/kernel/debug/binder/state 2>/dev/null | head -40
echo

echo "=== is vold even registered in servicemanager? ==="
service list 2>/dev/null | grep -i vold
echo

echo "=== crypto / storage props ==="
for p in ro.crypto.state ro.crypto.type vold.decrypt vold.post_fs_data_done \
         sys.listeners.registered persist.sys.vold_app_data_isolation_enabled \
         ro.vold.primary_physical ro.sys.sdcardfs; do
  echo "$p = $(getprop $p)"
done
echo

echo "=== sdcardfs / fuse in kernel? ==="
grep -iE 'sdcardfs|fuse' /proc/filesystems
echo
echo "=== /data mount ==="
grep -E ' /data | /mnt' /proc/mounts
echo

echo "=== dmesg tail for storage ==="
dmesg 2>/dev/null | grep -iE 'vold|sdcardfs|fuse|ext4.*data|selinux.*vold' | tail -30
echo DONE

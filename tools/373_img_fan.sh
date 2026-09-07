#!/bin/bash
# Add Pico's fan daemon to system-pn2-full.img.
#
# The GSI ships no fan control at all. The gpio-fan cooling device exists and
# accepts writes, but nothing drives it, so the fan never spins and the SoC sits
# above 80 C under VR load - a hardware risk, not just a performance one.
#
# fancontrol is a small self-contained /system/bin daemon: reads thermal_zone1..10
# and drives /sys/class/hwmon/hwmon1/pwm1{,_enable}. All eight of its DT_NEEDED
# libraries are already present. Its init rc needs an explicit seclabel because
# our build has no fancontrol domain (stock's policy defines fancontrol_exec).
set -u
IMG=/mnt/f/PN2Lineage/out/system-pn2-full.img
LOG=/mnt/f/PN2Lineage/notes/373_img_fan.txt
exec >"$LOG" 2>&1

fail=0
put() {
  local src="$1" dst="$2" mode="$3" dir base want got
  dir=$(dirname "$dst"); base=$(basename "$dst")
  [ -f "$src" ] || { printf '  MISSING SRC %s\n' "$src"; fail=$((fail+1)); return; }
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $src $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100$mode" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  want=$(stat -c%s "$src")
  got=$(debugfs -R "ls -l $dir" "$IMG" 2>/dev/null | awk -v b="$base" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then printf '  OK    %-44s %10s\n' "$dst" "$got"
  else printf '  FAIL  %-44s want %s got "%s"\n' "$dst" "$want" "${got:-absent}"; fail=$((fail+1)); fi
}

echo "=== free before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

echo
echo "=== fan daemon ==="
put /mnt/f/PN2Lineage/fan/fancontrol /bin/fancontrol 755
put /mnt/f/PN2Lineage/overlay/etc/init/pn2-fanservice.rc /etc/init/pn2-fanservice.rc 644

echo
echo "=== repair + verify ==="
e2fsck -fy "$IMG" 2>&1 | tail -4
if e2fsck -fn "$IMG" >/tmp/f3.txt 2>&1; then tail -2 /tmp/f3.txt; echo "  CLEAN"; else tail -6 /tmp/f3.txt; echo "  DIRTY"; fail=$((fail+1)); fi

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "FAN DAEMON ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

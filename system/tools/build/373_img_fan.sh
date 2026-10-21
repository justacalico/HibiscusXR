#!/bin/bash
HIBISCUS_ROOT="${HIBISCUS_ROOT:-${PN2_ROOT:-$HOME/PN2Lineage}}"
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
IMG=${HIBISCUS_ROOT}/out/system-pn2-full.img
LOG=${HIBISCUS_ROOT}/notes/373_img_fan.txt
exec >"$LOG" 2>&1

fail=0
. "$(dirname "${BASH_SOURCE[0]}")/../lib/img.sh"

echo "=== free before ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

echo
echo "=== fan daemon ==="
put ${HIBISCUS_ROOT}/fan/fancontrol /bin/fancontrol 755
put ${HIBISCUS_ROOT}/overlay/etc/init/pn2-fanservice.rc /etc/init/pn2-fanservice.rc 644

echo
echo "=== repair + verify ==="
img_fsck || fail=$((fail+1))

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'
echo
[ "$fail" -eq 0 ] && echo "FAN DAEMON ADDED OK" || echo "HAD $fail FAILURES"
echo DONE

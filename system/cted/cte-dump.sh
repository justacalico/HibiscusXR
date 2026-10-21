#!/system/bin/sh
# cte-dump - full headset debug dump for HCTE.
#
# Emits one tar.gz on stdout holding every section the "Dump Debug SSD"
# button writes into the zip the user sends back. Also runnable by hand:
#
#   adb shell cte-dump > dump.tgz
#
# Everything degrades: a failing probe lands its own stderr in its file
# and the run keeps going, so a dead subsystem still shows up in the zip
# as its error instead of a hole.
#
# The desktop-side fallback table (legacyDumpSections in
# applications/desktop/cte/lib/src/debug_dump.dart) mirrors the core of
# this list for headsets flashed before cte-dump existed - keep the two
# in step when sections change.

set +e
D=/data/local/tmp/cte-dump.$$
rm -rf "$D"
mkdir -p "$D/dumpsys" "$D/pkg" "$D/proc" "$D/sys" || exit 1

cap() { # cap <relpath> <cmd...>
  out="$D/$1"; shift
  ( "$@" ) >"$out" 2>&1
}

cap meta.txt sh -c 'date; echo; uptime; echo; uname -a; echo;
  for p in ro.product.model ro.product.device ro.serialno \
      ro.build.display.id ro.hibiscus.version persist.hibiscus.dof \
      persist.pn2.dof persist.hibiscus.cted ro.pvr.controller.service; do
    echo "$p = $(getprop $p)"; done'

cap getprop.txt getprop
cap logcat.txt logcat -d -v threadtime
cap logcat_events.txt logcat -d -b events
cap logcat_radio.txt logcat -d -b radio
cap logcat_crash.txt logcat -d -b crash
cap dmesg.txt dmesg
cap proc/last_kmsg.txt cat /proc/last_kmsg
cap proc/cmdline.txt cat /proc/cmdline
cap proc/meminfo.txt cat /proc/meminfo
cap proc/cpuinfo.txt cat /proc/cpuinfo
cap proc/interrupts.txt cat /proc/interrupts
cap proc/iomem.txt cat /proc/iomem
cap proc/mounts.txt cat /proc/mounts

cap dumpsys.txt dumpsys
cap dumpsys/_list.txt dumpsys -l
for s in $(dumpsys -l 2>/dev/null | tr -cd 'A-Za-z0-9_.-\n'); do
  cap "dumpsys/$s.txt" dumpsys "$s"
done

cap packages.txt pm list packages -f -u
cap permissions.txt pm list permissions -g -u -f
for p in $(pm list packages 2>/dev/null | sed 's/^package://' |
    grep -iE 'pico|pvr|hibiscus|neosalsa'); do
  cap "pkg/$p.txt" dumpsys package "$p"
done
cap appops.txt dumpsys appops
cap settings_global.txt settings list global
cap settings_system.txt settings list system
cap settings_secure.txt settings list secure

cap ps.txt ps -A
cap top.txt top -bn1
cap services.txt service list
cap df.txt df -h
cap selinux.txt sh -c 'getenforce; id'

cap sensors.txt dumpsys sensorservice
cap input.txt sh -c 'cat /proc/bus/input/devices; dumpsys input'
cap audio.txt sh -c 'dumpsys audio; dumpsys media.audio_policy'
cap display.txt sh -c 'dumpsys display; dumpsys SurfaceFlinger'
cap battery.txt sh -c 'dumpsys battery; dumpsys hardware_properties'
cap net.txt sh -c 'ip addr; ip route; netstat -an | head -80;
  dumpsys connectivity | head -200; dumpsys wifi | head -300'
cap bluetooth.txt dumpsys bluetooth_manager
cap dropbox.txt dumpsys dropbox

cap sys/spi.txt sh -c 'ls -lR /sys/bus/spi; ls -l /dev/spi*;
  for f in /sys/bus/spi/devices/*/name; do echo "== $f"; cat "$f"; done'
cap sys/sensors.txt sh -c 'ls -lR /sys/class/sensors;
  for f in /sys/class/sensors/*/name /sys/class/sensors/*/enable; do
    echo "== $f"; cat "$f"; done'
cap sys/pstore.txt sh -c 'ls -l /sys/fs/pstore;
  for f in /sys/fs/pstore/*; do echo "== $f"; cat "$f"; done'

cap ctrlshare.bin cat /sdcard/CtrlShareMem
cap hibiscus.txt sh -c 'ls -laR /data/local/tmp/hibiscus /data/local/tmp/xr;
  getprop | grep -iE "hibiscus|pvr|pico"'

cap tombstones.txt sh -c 'ls -l /data/tombstones; i=0;
  for t in /data/tombstones/*; do [ $i -ge 8 ] && break;
    echo "== $t"; head -c 65536 "$t"; echo; i=$((i+1)); done'
cap anr.txt sh -c 'ls -l /data/anr; i=0;
  for t in /data/anr/*; do [ $i -ge 8 ] && break;
    echo "== $t"; head -c 65536 "$t"; echo; i=$((i+1)); done'

cap etc.txt sh -c 'ls -l /system/etc/permissions /system/etc/default-permissions;
  for f in /system/etc/default-permissions/*; do echo "== $f"; cat "$f"; done'

( cd "$D" && tar -cf - . | gzip -c )
rm -rf "$D"

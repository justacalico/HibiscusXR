#!/system/bin/sh
# Resolve libSafetyAreaRecovery.so's whole dependency closure into /system/lib64.
#
# Only copy libraries that are genuinely ABSENT from /system - never shadow one
# that already exists. Dropping 8.1 vendor copies over Q system libraries has
# already broken libvintf (tinyxml2) and libandroidicu (ICU 60 vs 63) once.
set -u
mount -o rw,remount /system

resolve() {
  # is this soname already reachable by a /system process?
  for d in /system/lib64 /system/lib64/pvr_air /apex/com.android.runtime/lib64; do
    [ -f "$d/$1" ] && return 0
  done
  return 1
}

todo="libSafetyAreaRecovery.so"
done_list=""
added=""
round=0

while [ -n "$todo" ] && [ "$round" -lt 12 ]; do
  round=$((round+1))
  next=""
  for l in $todo; do
    case " $done_list " in *" $l "*) continue;; esac
    done_list="$done_list $l"

    if ! resolve "$l"; then
      src=$(find /vendor/lib64 -name "$l" 2>/dev/null | head -1)
      if [ -n "$src" ]; then
        cp -f "$src" "/system/lib64/$l"
        chmod 644 "/system/lib64/$l"
        chown root:root "/system/lib64/$l"
        chcon u:object_r:system_lib_file:s0 "/system/lib64/$l" 2>/dev/null
        added="$added $l"
        echo "  copied  $l  ($(stat -c%s "/system/lib64/$l") bytes)"
      else
        echo "  UNRESOLVED $l  (not in /vendor/lib64 either)"
        continue
      fi
    fi

    # queue this library's own DT_NEEDED
    f="/system/lib64/$l"
    [ -f "$f" ] || f=$(find /system/lib64 /apex/com.android.runtime/lib64 -name "$l" 2>/dev/null | head -1)
    [ -f "$f" ] || continue
    for d in $(strings -a "$f" 2>/dev/null | grep -E '^lib.*\.so$' | sort -u); do
      [ "$d" = "$l" ] && continue
      case " $done_list $next " in *" $d "*) continue;; esac
      next="$next $d"
    done
  done
  todo="$next"
done

echo
echo "=== added:${added:- nothing} ==="
sync

echo
echo "=== restart airservice and retry ==="
stop airservice; sleep 2; start airservice; sleep 8
echo "  airservice pid $(pidof airservice)"
logcat -c
am force-stop com.pvr.seethrough.setting
sleep 2
am start -n com.pvr.seethrough.setting/.MainActivity >/dev/null 2>&1
sleep 30

echo
echo "=== does the algorithm load now? ==="
logcat -d | grep -iE 'SafetyAreaRecovery|loadSymbols|startAlgorithm' | tail -8

echo
echo "=== boundary data ==="
ls -l /data/misc/user/0/boundary/stdata.txt /data/misc/user/0/boundary/stdataforalgorithm.txt
echo "  seethrough pid [$(pidof com.pvr.seethrough.setting)]"
echo "  segv=$(logcat -d | grep -c 'exited due to signal 11')"

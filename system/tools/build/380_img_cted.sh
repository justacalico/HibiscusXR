#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Compile and inject cted, the CTE daemon HCTE talks to.
#
# Generic OS feature - not tied to a headset vendor: the protocol only leans
# on getprop/dumpsys/screencap/logcat/pm plus the controller sharemem file
# when a device publishes one. Kept out of the image's own init policy by
# running in the shell domain and off unless persist.hibiscus.cted=1.
#
# Built from source here rather than shipped as a blob: everything in
# system/cted is ours. ctrl_state.c comes from vrhome - one decoder, no
# duplicated layouts.
set -u
IMG=${PN2_ROOT}/out/system-pn2-full.img
LOG=${PN2_ROOT}/notes/380_img_cted.txt
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
echo "=== build cted (aarch64) ==="
NDK="${ANDROID_NDK_LATEST_HOME:-$(ls -d "$ANDROID_HOME"/ndk/* 2>/dev/null | sort -V | tail -1)}"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang"
[ -x "$CC" ] || { echo "  MISSING NDK clang: $CC"; exit 1; }
"$CC" -O2 -DANDROID \
  -I"$PN2_ROOT/vrhome/src/input" \
  -o "$PN2_ROOT/cted/cted" \
  "$PN2_ROOT/cted/cted.c" \
  "$PN2_ROOT/vrhome/src/input/ctrl_state.c" \
  -lpthread || fail=$((fail+1))
ls -la "$PN2_ROOT/cted/cted" 2>/dev/null

echo
echo "=== inject ==="
put "$PN2_ROOT/cted/cted" /bin/cted 755
put "$PN2_ROOT/cted/hibiscus-cted.rc" /etc/init/hibiscus-cted.rc 644

echo
echo "=== repair + verify ==="
e2fsck -fy "$IMG" 2>&1 | tail -4
if e2fsck -fn "$IMG" >/tmp/f380.txt 2>&1; then tail -2 /tmp/f380.txt; echo "  CLEAN"; else tail -6 /tmp/f380.txt; echo "  DIRTY"; fail=$((fail+1)); fi

echo
echo "=== free after ==="
dumpe2fs -h "$IMG" 2>/dev/null | grep 'Free blocks'

if [ "$fail" -eq 0 ]; then echo "CTE DAEMON OK"; else echo "CTE DAEMON FAILED ($fail)"; fi
exit "$fail"

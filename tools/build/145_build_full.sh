#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Build system-pn2-full.img: the clean image plus every Pico component.
#
# Two images are maintained:
#   system-pn2.img       GSI + our fixes only. No proprietary content.
#   system-pn2-full.img  the same, plus Pico's stack. Ready to flash as-is.
#
# The GSI has ~57MB free and the Pico stack is ~313MB, so the filesystem has to
# be grown first. The system partition is 3943694336 bytes; stay under that.
set -u
CLEAN=${PN2_ROOT}/out/system-pn2.img
FULL=${PN2_ROOT}/out/system-pn2-full.img
STAGE=${PN2_ROOT}/fullstage
LOG=${PN2_ROOT}/notes/145_full.txt
PART_BYTES=3943694336
exec >"$LOG" 2>&1

echo "=== starting from the clean image ==="
cp -f "$CLEAN" "$FULL"
ls -l "$FULL"

echo
echo "=== grow to 3400M (partition is $PART_BYTES bytes) ==="
truncate -s 3400M "$FULL"
e2fsck -fy "$FULL" >/dev/null 2>&1
resize2fs "$FULL" 2>&1 | tail -3
df_before=$(dumpe2fs -h "$FULL" 2>/dev/null | grep -E 'Free blocks|Block count')
echo "$df_before"

echo
echo "=== writing the Pico stack ==="
cd "$STAGE" || exit 1

# create every directory first, deepest last
find . -type d | sed 's|^\./||' | grep -v '^\.$' | sort | while read -r d; do
  debugfs -w -R "mkdir /$d" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif /$d mode 040755" "$FULL" >/dev/null 2>&1
done
echo "  directories created: $(find . -type d | wc -l)"

ok=0; bad=0
while read -r f; do
  rel=${f#./}
  # rm first: debugfs `write` will NOT overwrite an existing file, it just fails
  # and leaves the original. /bin/vr and /framework/vr.jar already exist in the
  # GSI, so without this they silently keep the GSI's version.
  debugfs -w -R "rm /$rel" "$FULL" >/dev/null 2>&1
  debugfs -w -R "write $f /$rel" "$FULL" >/dev/null 2>&1
  # executables need the exec bit; everything else 644
  case "/$rel" in
    /bin/*) mode=0100755 ;;
    *)      mode=0100644 ;;
  esac
  debugfs -w -R "sif /$rel mode $mode" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif /$rel uid 0" "$FULL" >/dev/null 2>&1
  debugfs -w -R "sif /$rel gid 0" "$FULL" >/dev/null 2>&1
  want=$(stat -c%s "$f")
  got=$(debugfs -R "ls -l /$(dirname "$rel")" "$FULL" 2>/dev/null | awk -v b="$(basename "$rel")" '$NF==b {print $6}' | head -1)
  if [ "$got" = "$want" ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "  FAIL /$rel (want $want got ${got:-absent})"; fi
done < <(find . -type f)
echo "  files written: $ok ok, $bad failed"

echo
echo "=== spot check ==="
for p in /bin/pvrservice /lib64/libcompositor.pxr.so /lib64/lib6DofReset.so \
         /framework/pxr_sdk_api.jar /etc/pvr/slam/ORBvoc.bin \
         /priv-app/VRShell2/VRShell2.apk /priv-app/VRShell2/lib/arm64/libmain.so \
         /priv-app/PVRLauncher/PVRLauncher.apk /media/LoadingRes/inside_background_img.png; do
  sz=$(debugfs -R "ls -l $(dirname $p)" "$FULL" 2>/dev/null | awk -v b="$(basename $p)" '$NF==b {print $6}' | head -1)
  printf '  %-52s %s\n' "$p" "${sz:-MISSING}"
done

echo
echo "=== repair + verify ==="
e2fsck -fy "$FULL" >/dev/null 2>&1
if e2fsck -fn "$FULL" >/tmp/fsck_full.txt 2>&1; then
  tail -2 /tmp/fsck_full.txt
  echo "  CLEAN"
else
  tail -6 /tmp/fsck_full.txt
  echo "  STILL DIRTY"
fi

echo
sz=$(stat -c%s "$FULL")
echo "image: $sz bytes (partition $PART_BYTES)"
if [ "$sz" -gt "$PART_BYTES" ]; then echo "  ERROR: LARGER THAN THE PARTITION"; else echo "  fits"; fi
ls -l "$FULL"
echo DONE

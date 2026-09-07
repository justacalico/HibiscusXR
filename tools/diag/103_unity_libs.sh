#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Pull the app-side Pico runtime libraries.
#
# I skipped these on the first extraction as "~60MB pvrservice does not load" -
# correct for the daemon, wrong for the apps. VRShell is a Unity app and dies
# with UnsatisfiedLinkError: couldn't find "libPvr_UnitySDK.so".
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
OUT=${PN2_ROOT}/pvr_stack
LOG=${PN2_ROOT}/notes/103_unity.txt
exec >"$LOG" 2>&1

LIBS="libPvr_UnitySDK.so libPvr_UnitySDKExt1.so libPvr_UnitySDKExt5.so
      libPvr_UnitySDKExt8.so libPvr_UnitySDKExt9.so libPvr_UnitySDKExt10.so
      libPvr_UnitySDKExt11.so libPvr_UESDKExt2.so
      lib2dToVr.so libvraudio.so libpicologkit.so"

for d in lib64 lib; do
  mkdir -p "$OUT/$d"
  for f in $LIBS; do
    [ -f "$OUT/$d/$f" ] && continue
    debugfs -R "dump /$d/$f $OUT/$d/$f" "$IMG" 2>/dev/null
    if [ -s "$OUT/$d/$f" ]; then
      printf '  %-34s %10d\n' "$d/$f" "$(stat -c%s "$OUT/$d/$f")"
    else
      rm -f "$OUT/$d/$f"
      printf '  %-34s missing in image\n' "$d/$f"
    fi
  done
done

echo
echo "total staged: $(du -sh "$OUT" | cut -f1)"
echo DONE

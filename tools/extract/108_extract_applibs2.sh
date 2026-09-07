#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# rdump quietly did nothing when the destination arch dir already existed, so
# enumerate and dump each .so individually instead. Slower, but it reports
# per-file and cannot fail silently.
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
OUT=${PN2_ROOT}/pvr_applibs
LOG=${PN2_ROOT}/notes/108_applibs.txt
exec >"$LOG" 2>&1

PRIV="VRShell2 VRUserCenter2 pvrdisplay pvr_adapter CVService PVRVerify
      ShortcutMenu InitServer PicoSettingsProvider configserverservice"
APP="PxrNotification PicoToSvrService"

rm -rf "$OUT"; mkdir -p "$OUT"
total=0

grab() {   # grab <parent> <app>
  for arch in arm64 arm; do
    files=$(debugfs -R "ls -l /$1/$2/lib/$arch" "$IMG" 2>/dev/null \
            | awk '{print $NF}' | grep '\.so$')
    [ -z "$files" ] && continue
    mkdir -p "$OUT/$2/lib/$arch"
    for f in $files; do
      debugfs -R "dump /$1/$2/lib/$arch/$f $OUT/$2/lib/$arch/$f" "$IMG" 2>/dev/null
      if [ -s "$OUT/$2/lib/$arch/$f" ]; then
        printf '  %-20s %-6s %-32s %10d\n' "$2" "$arch" "$f" "$(stat -c%s "$OUT/$2/lib/$arch/$f")"
        total=$((total+1))
      else
        rm -f "$OUT/$2/lib/$arch/$f"
        printf '  %-20s %-6s %-32s FAILED\n' "$2" "$arch" "$f"
      fi
    done
  done
}

echo "=== priv-app ==="
for a in $PRIV; do grab priv-app "$a"; done
echo "=== app ==="
for a in $APP; do grab app "$a"; done

echo
echo "total .so extracted: $(find "$OUT" -name '*.so' | wc -l)"
echo "size: $(du -sh "$OUT" | cut -f1)"
echo DONE

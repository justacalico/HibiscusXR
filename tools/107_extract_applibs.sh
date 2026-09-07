#!/bin/bash
# Pull the app-private lib/ directories that sit NEXT TO each apk.
#
# My original extraction took the apk and oat/ and stopped. For system apps the
# native libs are laid down in lib/<arch>/ beside the apk instead of inside it,
# so every app-private .so was silently missing. VRShell ships its own libmain.so,
# libunity.so, libil2cpp.so and even its own libPvr_UnitySDK.so - which is why
# swapping the SYSTEM libPvr_UnitySDK.so appeared to help. That was a workaround
# for the real gap and should be reverted.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
OUT=/mnt/f/PN2Lineage/pvr_applibs
LOG=/mnt/f/PN2Lineage/notes/107_applibs.txt
exec >"$LOG" 2>&1

APPS="VRShell2 VRUserCenter2 pvrdisplay pvr_adapter CVService PVRVerify
      ShortcutMenu InitServer PicoSettingsProvider configserverservice"
APPS2="PxrNotification PicoToSvrService"

rm -rf "$OUT"; mkdir -p "$OUT"

pull_libs() {   # pull_libs <parent> <app>
  for arch in arm64 arm; do
    listing=$(debugfs -R "ls -l /$1/$2/lib/$arch" "$IMG" 2>/dev/null | grep -cE '\.so')
    if [ "$listing" -gt 0 ]; then
      mkdir -p "$OUT/$2/lib/$arch"
      debugfs -R "rdump /$1/$2/lib/$arch $OUT/$2/lib" "$IMG" 2>/dev/null
      n=$(find "$OUT/$2/lib/$arch" -name '*.so' 2>/dev/null | wc -l)
      sz=$(du -sh "$OUT/$2/lib" 2>/dev/null | cut -f1)
      printf '  %-22s %-6s %2s libs  %s\n' "$2" "$arch" "$n" "$sz"
    fi
  done
}

echo "=== priv-app ==="
for a in $APPS; do pull_libs priv-app "$a"; done
echo
echo "=== app ==="
for a in $APPS2; do pull_libs app "$a"; done
echo

echo "=== everything pulled ==="
find "$OUT" -name '*.so' -printf '%-70p %10s\n' | sed "s|$OUT/||"
echo
echo "total: $(find "$OUT" -name '*.so' | wc -l) libs, $(du -sh "$OUT" | cut -f1)"
echo DONE

#!/bin/bash
# VRShell needs libmain.so (Unity core) but VRShell2.apk contains no .so at all.
# For system apps the native libs are commonly laid down in a lib/ directory NEXT
# TO the apk rather than inside it. I only extracted the apk and oat/, so if that
# is the layout here, I missed every app-private native lib.
IMG=/mnt/f/PN2Lineage/images/ota_4.1.3/system.img
OUT=/mnt/f/PN2Lineage/pvr_applibs
LOG=/mnt/f/PN2Lineage/notes/106_applibs.txt
exec >"$LOG" 2>&1

echo "=== what is actually inside /priv-app/VRShell2 ==="
debugfs -R "ls -l /priv-app/VRShell2" "$IMG" 2>/dev/null
echo

for sub in lib lib/arm64 lib/arm; do
  echo "--- /priv-app/VRShell2/$sub ---"
  debugfs -R "ls -l /priv-app/VRShell2/$sub" "$IMG" 2>/dev/null
done
echo

echo "=== pull the whole VRShell2 dir so nothing else is missed ==="
rm -rf "$OUT"; mkdir -p "$OUT"
debugfs -R "rdump /priv-app/VRShell2 $OUT" "$IMG" 2>/dev/null
find "$OUT" -type f -printf '%-72p %10s\n' | sed "s|$OUT/||" | head -40
echo
echo "total files: $(find "$OUT" -type f | wc -l)  size: $(du -sh "$OUT" | cut -f1)"
echo
echo "=== is libmain.so anywhere in the image at all? ==="
for d in /lib64 /lib; do
  debugfs -R "ls -l $d" "$IMG" 2>/dev/null | grep -w 'libmain.so'
done
echo DONE

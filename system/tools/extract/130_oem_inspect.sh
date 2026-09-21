#!/system/bin/sh
# Do the /oem apps carry their own dex, or are they deodexed like the /system
# ones (in which case they need the same vdex unquicken treatment)?
exec 2>&1
for d in /oem/priv-app/*/; do
  n=$(basename "$d")
  a=$(ls "$d"*.apk 2>/dev/null | head -1)
  [ -n "$a" ] || continue
  oat=$(find "$d" -name '*.vdex' 2>/dev/null | wc -l)
  libs=$(find "$d" -name '*.so' 2>/dev/null | wc -l)
  echo "=== $n ==="
  echo "    apk    : $(stat -c%s "$a")"
  echo "    vdex   : $oat"
  echo "    applibs: $libs"
  find "$d" -type d | sed "s|$d|      |" | head -6
done
echo
echo "##### is there a classes.dex inside any of them #####"
for d in /oem/priv-app/*/; do
  n=$(basename "$d")
  a=$(ls "$d"*.apk 2>/dev/null | head -1)
  [ -n "$a" ] || continue
  # unzip -l is not on the device; look for the dex magic in the zip
  if grep -qa 'classes.dex' "$a" 2>/dev/null; then
    echo "  $n : has a classes.dex entry"
  else
    echo "  $n : NO classes.dex (deodexed)"
  fi
done
echo DONE

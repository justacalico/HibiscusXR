#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Assemble a staging tree that mirrors /system, containing every Pico component
# we have. This feeds the "full" image - the one that ships with the blobs in.
#
# Sources:
#   pvr_stack       daemons, libs, framework jars, etc/pvr   (from the OTA image)
#   pvr_apps_final  the 12 deodexed + platform-signed system apps
#   pvr_applibs     app-private lib/<arch>/ dirs
#   oem_final       the 5 deodexed + signed /oem apps
#   stock image     /media/LoadingRes, which VRShell loads its boot animation from
set -u
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
STAGE=${PN2_ROOT}/fullstage
APPS=${PN2_ROOT}/pvr_apps_final
APPLIBS=${PN2_ROOT}/pvr_applibs
OEM=${PN2_ROOT}/oem_final
STACK=${PN2_ROOT}/pvr_stack
LOG=${PN2_ROOT}/notes/144_stage.txt
exec >"$LOG" 2>&1

rm -rf "$STAGE"
mkdir -p "$STAGE"/{bin,lib,lib64,framework,etc/pvr,etc/init,priv-app,app,media}

echo "=== daemons ==="
for f in pvrservice qvrservice pvr_compute vr; do
  [ -f "$STACK/bin/$f" ] && cp "$STACK/bin/$f" "$STAGE/bin/" && echo "  bin/$f"
done

echo
echo "=== libs ==="
for a in lib lib64; do
  n=0
  for f in "$STACK/$a"/*.so; do
    [ -f "$f" ] || continue
    cp "$f" "$STAGE/$a/"; n=$((n+1))
  done
  echo "  $a: $n libs"
done

echo
echo "=== framework jars ==="
for f in "$STACK"/framework/*.jar; do
  [ -f "$f" ] && cp "$f" "$STAGE/framework/" && echo "  $(basename "$f")"
done

echo
echo "=== etc/pvr ==="
cp -r "$STACK/etc/pvr/." "$STAGE/etc/pvr/" 2>/dev/null
echo "  $(find "$STAGE/etc/pvr" -type f | wc -l) files, $(du -sh "$STAGE/etc/pvr" | cut -f1)"

echo
echo "=== system apps (signed) + their app-private libs ==="
for d in "$APPS"/*/; do
  n=$(basename "$d")
  apk=$(ls "$d"*.apk 2>/dev/null | head -1)
  [ -n "$apk" ] || continue
  # PxrNotification and PicoToSvrService live in /app, the rest in /priv-app
  case "$n" in
    PxrNotification|PicoToSvrService) sub=app ;;
    *) sub=priv-app ;;
  esac
  mkdir -p "$STAGE/$sub/$n"
  cp "$apk" "$STAGE/$sub/$n/"
  if [ -d "$APPLIBS/$n/lib" ]; then
    cp -r "$APPLIBS/$n/lib" "$STAGE/$sub/$n/"
    echo "  $sub/$n  (+$(find "$APPLIBS/$n/lib" -name '*.so' | wc -l) libs)"
  else
    echo "  $sub/$n"
  fi
done

echo
echo "=== oem-derived apps (signed) ==="
for d in "$OEM"/*/; do
  n=$(basename "$d")
  apk=$(ls "$d"*.apk 2>/dev/null | head -1)
  [ -n "$apk" ] || continue
  mkdir -p "$STAGE/priv-app/$n"
  cp "$apk" "$STAGE/priv-app/$n/"
  [ -d "$d/lib" ] && cp -r "$d/lib" "$STAGE/priv-app/$n/"
  echo "  priv-app/$n"
done

echo
echo "=== /media/LoadingRes (VRShell boot animation) ==="
debugfs -R "rdump /media/LoadingRes $STAGE/media" "$IMG" 2>/dev/null
if [ -d "$STAGE/media/LoadingRes" ]; then
  echo "  $(find "$STAGE/media/LoadingRes" -type f | wc -l) files, $(du -sh "$STAGE/media/LoadingRes" | cut -f1)"
else
  echo "  NOT FOUND in stock image"
fi

echo
echo "=== staging total ==="
find "$STAGE" -type f | wc -l
du -sh "$STAGE"
echo DONE

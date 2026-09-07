#!/bin/bash
# Which of stock's 22 whitelisted libs can actually link on our system?
#
# zygote preloads every entry in public.libraries.txt and aborts the boot if one
# fails, so a library may only be whitelisted once its ENTIRE DT_NEEDED closure is
# present. Resolve that closure offline against the device inventory.
set -u
LOG=/mnt/f/PN2Lineage/notes/193_closure.txt
exec >"$LOG" 2>&1
# the inventory is written by PowerShell, so it has CRLF line endings and a plain
# `grep -qx libc.so` never matches. Normalise before using it.
INV=/tmp/pn2_inventory.txt
tr -d '\r' < /mnt/f/PN2Lineage/notes/192_inventory.txt > "$INV"
echo "inventory: $(wc -l < "$INV") libs"

# candidate libs live in one of these two staging dirs
find_lib() {
  for d in /mnt/f/PN2Lineage/notes/lib64 /mnt/f/PN2Lineage/overlay_pvr/lib64; do
    [ -f "$d/$1" ] && { echo "$d/$1"; return; }
  done
}

needed() { readelf -dW "$1" 2>/dev/null | awk '/NEEDED/{gsub(/[\[\]]/,"",$5); print $5}'; }

have() { grep -qx "$1" "$INV"; }

WL="libpvrserviceclient.so libvirtualinputclient.so libpxrserviceclient.so
libairclient.so libSafetyArea.so libImageGrid.so libPvr_UnitySDK.so
libPvr_UnitySDKExt1.so libPvr_UnitySDKExt5.so libPvr_UnitySDKExt8.so
libPvr_UnitySDKExt9.so libPvr_UnitySDKExt10.so libPvr_UnitySDKExt11.so
libPvr_UESDKExt2.so libCVControllerClient.pxr.so lib6DofReset.so
libpxrnotification.pxr.so libconfigurationclient.pxr.so libplugin.pxr.so
libloader.pxr.so libruntime.pxr.so libcompositor.pxr.so"

echo "=== per-library dependency check ==="
SAFE=""; UNSAFE=""; NOFILE=""
for l in $WL; do
  f=$(find_lib "$l")
  if [ -z "$f" ]; then
    if have "$l"; then echo "  ?  $l  (on device but not staged locally - cannot check)"; NOFILE="$NOFILE $l"
    else echo "  -  $l  (absent everywhere; stock whitelists it anyway)"; fi
    continue
  fi
  miss=""
  for n in $(needed "$f"); do
    have "$n" || miss="$miss $n"
  done
  if [ -z "$miss" ]; then
    echo "  OK $l"
    SAFE="$SAFE $l"
  else
    echo "  XX $l   MISSING:$miss"
    UNSAFE="$UNSAFE $l"
  fi
done

echo
echo "=== safe to whitelist ==="
for l in $SAFE; do echo "  $l"; done
echo
echo "=== NOT safe (would abort zygote) ==="
for l in $UNSAFE; do echo "  $l"; done
echo
echo "=== unverifiable (not staged) ==="
for l in $NOFILE; do echo "  $l"; done
echo DONE

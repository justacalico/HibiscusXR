#!/usr/bin/env bash
# Install the app-private native libs next to each apk, and undo the system
# libPvr_UnitySDK.so swap.
#
# That swap was a workaround for VRShell's missing app-private lib dir. VRShell
# ships its own libPvr_UnitySDK.so (2051960 bytes, matching no system variant),
# so with lib/arm64 in place the system copy goes back to stock.
#
# Usage: install_applibs.sh [pvr_applibs-dir] [pvr_apps-dir]
#   PN2_SERIAL  device serial if more than one is connected
set -euo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
SRC="${1:-$PN2_ROOT/pvr_applibs}"
APPS="${2:-$PN2_ROOT/pvr_apps}"
ADB=(adb ${PN2_SERIAL:+-s "$PN2_SERIAL"})

command -v adb >/dev/null || { echo "adb not in PATH" >&2; exit 1; }
[ -d "$SRC" ] || { echo "no such dir: $SRC" >&2; exit 1; }

# where each app lives (app/ vs priv-app/)
declare -A loc
while IFS= read -r apk; do
  loc[$(basename "$(dirname "$apk")")]=$(basename "$(dirname "$(dirname "$apk")")")
done < <(find "$APPS" -name '*.apk' 2>/dev/null)

"${ADB[@]}" shell rm -rf /data/local/tmp/applibs >/dev/null 2>&1 || true
"${ADB[@]}" push "$SRC" /data/local/tmp/applibs | tail -1

sh=$(mktemp)
{
  echo '#!/system/bin/sh'
  echo 'exec 2>&1'
  echo 'mount -o rw,remount /system'
  for app in "$SRC"/*/; do
    n=$(basename "$app")
    where="${loc[$n]:-priv-app}"
    dest="/system/$where/$n"
    echo "rm -rf $dest/lib"
    # adb push into a NON-existent target puts the contents directly in it, with
    # no extra source-dir level - hence no applibs/ component here
    echo "cp -r /data/local/tmp/applibs/$n/lib $dest/lib"
    echo "chown -R root:root $dest/lib"
    echo "find $dest/lib -type d -exec chmod 755 {} \\;"
    echo "find $dest/lib -type f -exec chmod 644 {} \\;"
  done
  # put the system SDK lib back the way it shipped
  echo 'for a in lib64 lib; do'
  echo '  if [ -f /system/$a/libPvr_UnitySDK.so.orig ]; then'
  echo '    cp -f /system/$a/libPvr_UnitySDK.so.orig /system/$a/libPvr_UnitySDK.so'
  echo '    rm -f /system/$a/libPvr_UnitySDK.so.orig'
  echo '    echo "reverted /system/$a/libPvr_UnitySDK.so to stock"'
  echo '  fi'
  echo 'done'
  echo 'sync'
  echo 'mount -o ro,remount /system'
  echo 'echo "--- app libs installed ---"'
  echo 'for d in /system/priv-app/*/lib /system/app/*/lib; do'
  echo '  [ -d "$d" ] && echo "$d: $(find $d -name "*.so" | wc -l) libs"'
  echo 'done'
} > "$sh"

"${ADB[@]}" push "$sh" /data/local/tmp/applibs.sh >/dev/null
rm -f "$sh"
"${ADB[@]}" shell "chmod 755 /data/local/tmp/applibs.sh; su -c /data/local/tmp/applibs.sh || /system/xbin/su -c /data/local/tmp/applibs.sh"

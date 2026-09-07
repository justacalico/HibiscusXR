#!/usr/bin/env bash
# Install the deodexed /oem apps into /system/priv-app.
#
# /oem itself is left untouched. Android 10 does not register anything from that
# partition on this build (stock 8.1 does), and rather than chase why, put them
# somewhere PackageManager definitely scans - which is also where they would
# live in an image we build ourselves.
#
# Usage: install_oem_apps.sh [oem_final-dir]
#   PN2_SERIAL  device serial if more than one is connected
set -euo pipefail

PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
FIN="${1:-$PN2_ROOT/oem_final}"
ADB=(adb ${PN2_SERIAL:+-s "$PN2_SERIAL"})

command -v adb >/dev/null || { echo "adb not in PATH" >&2; exit 1; }
[ -d "$FIN" ] || { echo "no such dir: $FIN" >&2; exit 1; }

"${ADB[@]}" shell rm -rf /data/local/tmp/oemapps >/dev/null 2>&1 || true
"${ADB[@]}" push "$FIN" /data/local/tmp/oemapps | tail -1

sh=$(mktemp)
{
  echo '#!/system/bin/sh'
  echo 'exec 2>&1'
  echo 'mount -o rw,remount /system'
  for app in "$FIN"/*/; do
    n=$(basename "$app")
    echo "rm -rf /system/priv-app/$n"
    echo "cp -r /data/local/tmp/oemapps/$n /system/priv-app/$n"
    echo "chown -R root:root /system/priv-app/$n"
    echo "find /system/priv-app/$n -type d -exec chmod 755 {} \\;"
    echo "find /system/priv-app/$n -type f -exec chmod 644 {} \\;"
  done
  echo 'sync'
  echo 'mount -o ro,remount /system'
  echo 'echo "--- installed ---"'
  echo 'for d in /system/priv-app/*; do'
  echo '  [ -d "$d" ] && echo "  $(basename $d): $(ls $d/*.apk 2>/dev/null | wc -l) apk"'
  echo 'done'
} > "$sh"

"${ADB[@]}" push "$sh" /data/local/tmp/oeminstall.sh >/dev/null
rm -f "$sh"
"${ADB[@]}" shell "chmod 755 /data/local/tmp/oeminstall.sh; su -c /data/local/tmp/oeminstall.sh || /system/xbin/su -c /data/local/tmp/oeminstall.sh"

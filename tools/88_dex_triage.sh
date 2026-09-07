#!/system/bin/sh
# Is the odex rejected because we RE-SIGNED the apk, or for a more fundamental
# reason? Three apps were installed WITHOUT re-signing (no sharedUserId):
# PicoToSvrService, InitServer, VRUserCenter2. If those fail too, re-signing is
# not the cause.
echo "=== which packages hit ClassNotFound this boot ==="
logcat -d 2>/dev/null | grep 'ClassNotFoundException' \
  | grep -oE '/system/(priv-)?app/[A-Za-z0-9_]+/' | sort | uniq -c | sort -rn
echo
echo "=== re-signed vs as-is ==="
echo "  re-signed : PicoSettingsProvider configserverservice CVService pvrdisplay"
echo "              pvr_adapter PxrNotification PVRVerify ShortcutMenu VRShell2"
echo "  as-is     : PicoToSvrService InitServer VRUserCenter2"
echo
echo "=== do the AS-IS ones appear in the failures above? ==="
for a in PicoToSvrService InitServer VRUserCenter2; do
  n=$(logcat -d 2>/dev/null | grep -c "$a")
  echo "  $a : $n log lines mentioning it"
done
echo
echo "=== oat files actually present and readable? ==="
ls -l /system/priv-app/VRShell2/oat/arm64/ 2>&1
ls -l /system/app/PxrNotification/oat/arm64/ 2>&1
echo
echo "=== what does ART say about the oat specifically ==="
logcat -d 2>/dev/null | grep -iE 'oat file|OatFileAssistant|vdex|odex|dex2oat|Failed to open oat|boot image' | tail -20
echo DONE

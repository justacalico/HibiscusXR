#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Pull the Pico VR apps out of the stock system image.
#
# Inspect before installing. Pico's apps are signed with Pico's platform key; the
# GSI is signed with test-keys. Anything declaring sharedUserId="android.uid.system"
# cannot be granted that uid here - the signatures do not match and PackageManager
# will refuse it. Knowing which apps are in that category decides the whole
# approach, so find out first.
IMG=${PN2_ROOT}/images/ota_4.1.3/system.img
OUT=${PN2_ROOT}/pvr_apps
LOG=${PN2_ROOT}/notes/77_apps.txt
exec >"$LOG" 2>&1

rm -rf "$OUT"; mkdir -p "$OUT/priv-app" "$OUT/app" "$OUT/etc/permissions"

# The VR-relevant set. Deliberately not everything: no telephony, no IME, no
# Pico store/account apps we do not need to prove the runtime works.
PRIV="VRShell2 VRUserCenter2 VRWing2 pvr_adapter pvrdisplay CVService PVRVerify
      seethroughsetting configserverservice assistantHmd ControllerUpgrade
      PicoSettingsProvider ShortcutMenu InitServer tobSettings StreamingAssistant"
APP="PxrNotification PicoToSvrService WebVR"

for d in $PRIV; do
  debugfs -R "rdump /priv-app/$d $OUT/priv-app" "$IMG" 2>/dev/null
done
for d in $APP; do
  debugfs -R "rdump /app/$d $OUT/app" "$IMG" 2>/dev/null
done

# permission allowlists - a privileged app whose permissions are not allowlisted
# will be refused on Android 10 (it was only a warning on 8.1)
for f in privapp-permissions-platform.xml privapp-permissions-qti.xml \
         privapp-permissions-com.qualcomm.location.xml; do
  debugfs -R "dump /etc/permissions/$f $OUT/etc/permissions/$f" "$IMG" 2>/dev/null
done

echo "=== what we got ==="
find "$OUT" -name '*.apk' -printf '%-58p %10s\n' | sed "s|$OUT/||"
echo
echo "total: $(find "$OUT" -name '*.apk' | wc -l) apks, $(du -sh "$OUT" | cut -f1)"
echo DONE

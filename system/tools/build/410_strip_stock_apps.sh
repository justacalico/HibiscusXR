#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
# Strip the stock GSI packages a WiFi-only headset will never use:
# telephony, contacts/mail, 2D media apps, printing/backup leftovers,
# NFC, Lineage extras, the launcher and the theme/navbar/cutout
# overlays, CTS shims. Deletion is real - dirs leave the image and the
# blocks come back, not a pm disable.
#
# Runs on the CLEAN image right after 143. system-pn2-full.img is a
# byte copy of it made in 145, so one pass strips both outputs.
#
# Package names do not match on-disk dir names (com.android.dialer is
# /priv-app/Dialer), so the script walks the image's app roots, reads
# each APK's package name with aapt and kills the ones on the list.
set -u
IMG=${PN2_ROOT}/out/system-pn2.img
LOG=${PN2_ROOT}/notes/410_strip.txt
exec >"$LOG" 2>&1

fail=0
killed=0

BT=$(ls -d "${ANDROID_SDK_ROOT:-$ANDROID_HOME}"/build-tools/* 2>/dev/null | sort -V | tail -1)
AAPT="$BT/aapt"
[ -x "$AAPT" ] || { echo "ERROR: no aapt at $AAPT"; echo "STRIP HAD FAILURES"; exit 1; }

# ------------------------------------------------------------------ lists
# Exact package kills, grouped like the work item.
EXACT=" \
com.android.dialer com.android.phone com.android.server.telecom \
com.android.messaging com.android.mms.service com.android.smspush \
com.android.stk com.android.simappdialog \
com.android.cellbroadcastreceiver com.android.carrierconfig \
com.android.carrierdefaultapp com.android.ons com.android.service.ims \
com.android.service.ims.presence com.android.calllogbackup \
com.android.emergency com.android.providers.telephony \
com.android.providers.blockednumber \
me.phh.treble.overlay.telephony.lte \
com.android.contacts com.android.providers.contacts \
com.android.providers.calendar com.android.email com.android.exchange \
org.lineageos.etar \
com.android.camera2 com.android.gallery3d com.android.deskclock \
com.android.calculator2 com.android.dreams.basic \
com.android.dreams.phototable com.android.wallpaper \
com.android.wallpapercropper com.android.wallpaperbackup \
com.android.wallpaper.livepicker org.lineageos.eleven \
org.lineageos.recorder org.lineageos.jelly org.lineageos.backgrounds \
com.android.egg com.android.bookmarkprovider \
com.android.providers.partnerbookmarks \
com.android.printspooler com.android.bips \
com.android.printservice.recommendation com.android.mtp \
com.stevesoltys.seedvault com.android.backupconfirm \
com.android.localtransport com.android.sharedstoragebackup \
com.android.managedprovisioning com.android.companiondevicemanager \
com.android.dynsystem com.android.onetimeinitializer \
com.android.terminal com.android.traceur com.android.hotspot2 \
com.android.safetyregulatoryinfo \
com.android.nfc com.android.se com.android.apps.tag \
org.lineageos.updater org.lineageos.setupwizard \
org.lineageos.profiles org.lineageos.customization \
org.lineageos.audiofx \
com.android.launcher3 \
com.android.internal.display.cutout.emulation.corner \
com.android.internal.display.cutout.emulation.double \
com.android.internal.display.cutout.emulation.tall \
com.android.cts.ctsshim com.android.cts.priv.ctsshim"

# Prefix kills: theme accents/fonts/icon packs, Lineage overlays, the
# navbar-mode overlays. Cutout 'none' stays - it is the one overlay we
# actually want applied.
PREFIXES="com.android.theme. org.lineageos.overlay. \
com.android.internal.systemui.navbar."

# phh treble overlays for other phones and dead features. Their package
# names vary a little between GSI builds, so any me.phh.treble.overlay.*
# carrying one of these tokens dies.
PHH_TOKENS="telephony nokia xiaomi aod gestures devinputjack navbar \
nightmode falselocks tethering webview cafims mtkims slsiims sprdims"

# Must still be in the image when the pass ends - if one of these is
# gone the match rules went too wide.
KEEP="com.android.settings com.android.systemui \
com.android.packageinstaller com.android.permissioncontroller \
com.android.documentsui com.android.providers.downloads \
com.android.providers.media com.android.webview com.android.bluetooth \
com.android.inputmethod.latin com.android.captiveportallogin \
com.android.storagemanager com.android.networkstack"

want_dead() {
  local p="$1" x t
  for x in $EXACT; do [ "$p" = "$x" ] && return 0; done
  case "$p" in
    com.android.theme.*|org.lineageos.overlay.*|com.android.internal.systemui.navbar.*)
      return 0 ;;
    me.phh.treble.overlay.*)
      for t in $PHH_TOKENS; do
        case "$p" in *"$t"*) return 0 ;; esac
      done ;;
  esac
  return 1
}

# ------------------------------------------------------------- img walks
# debugfs exits 0 even when a path does not resolve - the only signal
# is the "File not found by ext2_lookup" text it prints.
img_exists() {
  ! debugfs -R "stat $1" "$IMG" 2>&1 | grep -q ext2_lookup
}

# debugfs `ls -l` fields: inode mode (links) uid gid size date time name.
img_ls() { # img_ls <dir> -> "mode name" per entry
  debugfs -R "ls -l $1" "$IMG" 2>/dev/null | \
    awk 'NF >= 8 && $2 ~ /^[0-9]+$/ {print $2, $NF}'
}

rmrf() { # rmrf <img-dir>: delete a directory tree inside the image
  local d="$1" mode name
  img_ls "$d" | while read -r mode name; do
    case "$name" in .|..) continue ;; esac
    case "$mode" in
      4*|04*) rmrf "$d/$name" ;;
      *)      debugfs -w -R "rm $d/$name" "$IMG" >/dev/null 2>&1 ;;
    esac
  done
  debugfs -w -R "rmdir $d" "$IMG" >/dev/null 2>&1
}

find_apks() { # find_apks <img-dir>: print every *.apk path under it
  local d="$1" mode name
  img_ls "$d" | while read -r mode name; do
    case "$name" in .|..) continue ;; esac
    case "$mode" in
      4*|04*) find_apks "$d/$name" ;;
      *)      case "$name" in *.apk) echo "$d/$name" ;; esac ;;
    esac
  done
}

pkg_seen() { # pkg_seen <pkg> <file>: is $pkg column 1 of <file>
  awk -F'\t' -v p="$1" '$1 == p {found=1} END {exit !found}' "$2"
}

# ------------------------------------------------------------------ main
ROOTS="/app /priv-app /product/app /product/priv-app \
/product/overlay /overlay /system/app /system/priv-app"

echo "=== enumerating apks in the image ==="
TMP=$(mktemp -d)
: > "$TMP/pkgs.txt"        # package<TAB>apk-path
: > "$TMP/kill.txt"        # package<TAB>apk-path, to be removed
for root in $ROOTS; do
  img_exists "$root" || continue
  echo "  scanning $root"
  find_apks "$root" > "$TMP/apks.txt"
  while read -r apk; do
    [ -n "$apk" ] || continue
    debugfs -R "dump $apk $TMP/a.apk" "$IMG" >/dev/null 2>&1
    pkg=$("$AAPT" dump badging "$TMP/a.apk" 2>/dev/null | \
      sed -n "s/^package: name='\([^']*\)'.*/\1/p" | head -1)
    [ -n "$pkg" ] || { echo "  WARN no package name for $apk"; continue; }
    printf '%s\t%s\n' "$pkg" "$apk" >> "$TMP/pkgs.txt"
    if want_dead "$pkg"; then
      printf '%s\t%s\n' "$pkg" "$apk" >> "$TMP/kill.txt"
    fi
  done < "$TMP/apks.txt"
done
echo "  found $(wc -l < "$TMP/pkgs.txt") packages, $(wc -l < "$TMP/kill.txt") on the kill list"

echo
echo "=== deleting killed packages ==="
while IFS=$'\t' read -r pkg apk; do
  dir=$(dirname "$apk")
  case " $ROOTS " in
    *" $dir "*)
      # loose overlay apk sitting directly in a scanned root
      debugfs -w -R "rm $apk" "$IMG" >/dev/null 2>&1
      debugfs -w -R "rm $apk.idsig" "$IMG" >/dev/null 2>&1 ;;
    *)
      rmrf "$dir" ;;
  esac
  if img_exists "$apk"; then
    echo "  FAIL  $pkg ($apk still present)"
    fail=$((fail+1))
  else
    echo "  rm    $pkg ($dir)"
    killed=$((killed+1))
  fi
done < "$TMP/kill.txt"

echo
echo "=== kill-list coverage (informational) ==="
for x in $EXACT; do
  pkg_seen "$x" "$TMP/pkgs.txt" || echo "  not in image: $x"
done

echo
echo "=== keep-list check ==="
for k in $KEEP; do
  if pkg_seen "$k" "$TMP/pkgs.txt" && ! pkg_seen "$k" "$TMP/kill.txt"; then
    echo "  OK    $k still installed"
  else
    echo "  FAIL  $k missing or killed"
    fail=$((fail+1))
  fi
done

echo
echo "=== repair pass ==="
e2fsck -fy "$IMG" 2>&1 | tail -6

echo
echo "=== fsck after repair (must be clean) ==="
if e2fsck -fn "$IMG" > "$TMP/fsck.txt" 2>&1; then
  tail -2 "$TMP/fsck.txt"
  echo "  filesystem CLEAN"
else
  tail -8 "$TMP/fsck.txt"
  echo "  filesystem STILL DIRTY"
  fail=$((fail+1))
fi
rm -rf "$TMP"

echo
ls -l "$IMG"
echo "  stripped $killed packages"
echo
if [ "$fail" -eq 0 ]; then echo "STRIP OK"; else echo "STRIP HAD $fail FAILURES"; fi
echo DONE

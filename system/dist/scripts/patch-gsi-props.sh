#!/usr/bin/env bash
# First-boot adb props, applied to gsi_raw.img through debugfs - same result
# as tools/build/26_patch_gsi_adb.sh but without a loop mount (no root needed,
# matching the debugfs convention the rest of the pipeline uses).
#
# Fresh /data has no persisted USB config, so default the gadget to mtp+adb
# on every build. Non-release builds also drop adb auth for bring-up; a
# CHANNEL=release build ships with ro.secure/ro.debuggable/ro.adb.secure at
# their secure defaults. Order matters: init only has triggers for "adb" or
# "X,adb" - "adb,mtp" matches nothing and the gadget never comes up.
set -euo pipefail
R="${HIBISCUS_ROOT:?set HIBISCUS_ROOT to a build root - see system/dist/README.md}"
IMG="$R/gsi/gsi_raw.img"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

want_insecure=1
[ "${CHANNEL:-alpha}" = "release" ] && want_insecure=0

patch_prop() { # patch_prop <img-path>
  local dst="$1" t="$T/$(basename "$1")"
  debugfs -R "dump $dst $t" "$IMG" 2>/dev/null
  [ -s "$t" ] || { echo "  no $dst in image"; return 0; }
  local has_insecure=0
  grep -q '^ro\.secure=0' "$t" && has_insecure=1
  if grep -q 'hibiscus usb config' "$t" && [ "$has_insecure" = "$want_insecure" ]; then
    echo "  $dst already patched"; return 0
  fi
  sed -i -E '/^(persist\.sys\.usb\.config|ro\.adb\.secure|ro\.debuggable|ro\.secure)=/d;/adb bring-up/d;/hibiscus usb config/d' "$t"
  [ -n "$(tail -c1 "$t")" ] && echo "" >> "$t" || true
  cat >> "$t" <<'EOF'

# --- hibiscus usb config ---
# Fresh /data has no persisted USB config, so default the gadget to mtp+adb.
persist.sys.usb.config=mtp,adb
EOF
  if [ "$want_insecure" = 1 ]; then
    cat >> "$t" <<'EOF'
# Bring-up only; release builds never ship these.
ro.adb.secure=0
ro.debuggable=1
ro.secure=0
EOF
  fi
  debugfs -w -R "rm $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "write $t $dst" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst mode 0100600" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst uid 0" "$IMG" >/dev/null 2>&1
  debugfs -w -R "sif $dst gid 0" "$IMG" >/dev/null 2>&1
  echo "  $dst patched"
}

patch_prop /build.prop
patch_prop /etc/prop.default

e2fsck -fy "$IMG" >/dev/null 2>&1
e2fsck -fn "$IMG" >/dev/null 2>&1 || { echo "fsck dirty after prop patch" >&2; exit 1; }
grep -E 'persist\.sys\.usb|ro\.(adb\.secure|debuggable|secure)' "$T/build.prop"

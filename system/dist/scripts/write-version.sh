#!/usr/bin/env bash
# Stamp the build version into gsi_raw.img before the 143/145 steps copy it
# into both outputs. Two surfaces:
#
#   ro.hibiscus.version in /build.prop  - adb shell getprop, for triage
#   /etc/hibiscus-release               - what the settings app reads; a plain
#                                         file because apps cannot read custom
#                                         ro.* props on Android 10 without a
#                                         declared property context
#
# HIBISCUS_VERSION is the release tag the GitHub workflow computed
# (e.g. alpha-v2026.09.22-r7); local runs fall back to dev-<date>.
set -euo pipefail
R="${PN2_ROOT:?}"
IMG="$R/gsi/gsi_raw.img"
VERSION="${HIBISCUS_VERSION:-dev-$(date -u +%Y%m%d)}"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

echo "version: $VERSION"

# --- /build.prop -----------------------------------------------------------
debugfs -R "dump /build.prop $T/build.prop" "$IMG" 2>/dev/null
[ -s "$T/build.prop" ] || { echo "FAILED: no /build.prop in image" >&2; exit 1; }
sed -i '/^ro\.hibiscus\.version=/d' "$T/build.prop"
[ -n "$(tail -c1 "$T/build.prop")" ] && echo "" >> "$T/build.prop" || true
cat >> "$T/build.prop" <<EOF

# --- Hibiscus version ------------------------------------------------------
ro.hibiscus.version=$VERSION
EOF
debugfs -w -R "rm /build.prop" "$IMG" >/dev/null 2>&1
debugfs -w -R "write $T/build.prop /build.prop" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop mode 0100600" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop uid 0" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /build.prop gid 0" "$IMG" >/dev/null 2>&1

# --- /etc/hibiscus-release --------------------------------------------------
echo "$VERSION" > "$T/hibiscus-release"
debugfs -w -R "rm /etc/hibiscus-release" "$IMG" >/dev/null 2>&1
debugfs -w -R "write $T/hibiscus-release /etc/hibiscus-release" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /etc/hibiscus-release mode 0100644" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /etc/hibiscus-release uid 0" "$IMG" >/dev/null 2>&1
debugfs -w -R "sif /etc/hibiscus-release gid 0" "$IMG" >/dev/null 2>&1

# --- verify -----------------------------------------------------------------
debugfs -R "dump /build.prop $T/check.prop" "$IMG" 2>/dev/null
grep -q "^ro\.hibiscus\.version=$VERSION$" "$T/check.prop" \
  || { echo "FAILED: prop not in image" >&2; exit 1; }
debugfs -R "dump /etc/hibiscus-release $T/check.rel" "$IMG" 2>/dev/null
grep -q "^$VERSION$" "$T/check.rel" \
  || { echo "FAILED: release file not in image" >&2; exit 1; }
e2fsck -fy "$IMG" >/dev/null 2>&1
e2fsck -fn "$IMG" >/dev/null 2>&1 || { echo "fsck dirty after version stamp" >&2; exit 1; }
echo "  stamped ro.hibiscus.version + /etc/hibiscus-release"

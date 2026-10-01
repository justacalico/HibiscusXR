#!/usr/bin/env bash
# 144_stage_full.sh reads /media/LoadingRes (VRShell's boot animation) out of
# the stock OTA system.img. The real thing is 3.6G and carries nothing else we
# need, so fake just that subtree in a small ext4 image - rdump can't tell the
# difference.
set -euo pipefail
R="${PN2_ROOT:?}"
SRC="$R/.stub"
IMG="$R/images/ota_4.1.3/system.img"
HERE="$(cd "$(dirname "$0")" && pwd)"

[ -d "$SRC/media/LoadingRes" ] || { echo "$SRC/media/LoadingRes missing" >&2; exit 1; }
mkdir -p "$(dirname "$IMG")"

# The loadingres package carries the stock set - the branded copy lives in the
# monorepo and replaces it outright so VRShell's loading screen shows Hibiscus
# art rather than Pico's (or nothing, if the package ever ships a bare
# config.txt again).
RES="$HERE/../../fullstage/media/LoadingRes"
[ -d "$RES" ] || { echo "$RES missing" >&2; exit 1; }
rm -rf "$SRC/media/LoadingRes"
cp -a "$RES" "$SRC/media/LoadingRes"
echo "LoadingRes overlaid from repo: $(find "$SRC/media/LoadingRes" -type f | wc -l) files"

mb=$(( $(du -sm "$SRC" | cut -f1) * 2 + 16 ))
dd if=/dev/zero of="$IMG" bs=1M count="$mb" status=none
mke2fs -q -F -t ext4 -d "$SRC" "$IMG"
echo "stub stock image: $IMG (${mb}M)"
debugfs -R "ls /media/LoadingRes" "$IMG" 2>/dev/null | head -5

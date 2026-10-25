#!/usr/bin/env bash
# Test the image property verifier without Android build inputs or a headset.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
VERIFY="$ROOT/system/dist/scripts/verify-build-props.sh"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

command -v debugfs >/dev/null
command -v mke2fs >/dev/null

make_image() {
  local path="$1" contents="$2"
  truncate -s 16M "$path"
  mke2fs -q -t ext2 -F "$path"
  printf '%s\n' "$contents" > "$TMP/build.prop"
  debugfs -w -R "write $TMP/build.prop /build.prop" "$path" >/dev/null 2>&1
}

valid="$TMP/valid.img"
make_image "$valid" $'ro.build.product=example\nro.product.device=example\nro.product.model=Example'
"$VERIFY" "$valid"

missing="$TMP/missing.img"
make_image "$missing" $'ro.build.product=example\nro.product.model=Example'
if "$VERIFY" "$missing" >/dev/null 2>&1; then
  echo 'FAIL: missing device property passed validation' >&2
  exit 1
fi

glued="$TMP/glued.img"
make_image "$glued" $'ro.build.product=example\nro.sf.hwrotation=90ro.product.device=example\nro.product.model=Example'
if "$VERIFY" "$glued" >/dev/null 2>&1; then
  echo 'FAIL: glued property passed validation' >&2
  exit 1
fi

echo 'PASS: valid build properties accepted; missing and glued properties rejected'

# Reproduce the device image writer's exact append sequence with the tracked
# props.append source. The source currently ends without a newline, so the
# writer must insert one before adding product identity.
writer="$TMP/writer.img"
truncate -s 16M "$writer"
mke2fs -q -t ext2 -F "$writer"
printf 'ro.build.product=example\n' > "$TMP/build.prop"
cat "$ROOT/drivers/pn2/image/props.append" >> "$TMP/build.prop"
[ -n "$(tail -c1 "$TMP/build.prop")" ] && echo '' >> "$TMP/build.prop"
printf 'ro.product.device=example\nro.product.model=Example\n' >> "$TMP/build.prop"
debugfs -w -R "write $TMP/build.prop /build.prop" "$writer" >/dev/null 2>&1
"$VERIFY" "$writer"

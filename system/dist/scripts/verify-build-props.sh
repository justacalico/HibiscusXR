#!/usr/bin/env bash
# Verify the identity properties that Android uses to select device services.
set -euo pipefail

IMG="${1:?usage: $0 <system-image>}"
[ -f "$IMG" ] || { echo "image not found: $IMG" >&2; exit 1; }

TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
debugfs -R "dump /build.prop $TMP" "$IMG" >/dev/null 2>&1 || {
  echo "could not extract /build.prop from $IMG" >&2
  exit 1
}

for prop in ro.build.product ro.product.device ro.product.model; do
  count=$(grep -c "^$prop=" "$TMP" || true)
  [ "$count" -eq 1 ] || {
    echo "$IMG: expected one $prop, found $count" >&2
    exit 1
  }
  value=$(sed -n "s/^$prop=//p" "$TMP")
  [ -n "$value" ] || { echo "$IMG: empty $prop" >&2; exit 1; }
done

build_product=$(sed -n 's/^ro.build.product=//p' "$TMP")
product_device=$(sed -n 's/^ro.product.device=//p' "$TMP")
[ "$build_product" = "$product_device" ] || {
  echo "$IMG: ro.build.product and ro.product.device disagree" >&2
  exit 1
}

if grep -qE '^[^#[:space:]].*=.*[[:alnum:]_]+\.[[:alnum:]_]+=' "$TMP"; then
  echo "$IMG: malformed glued build.prop properties:" >&2
  grep -nE '^[^#[:space:]].*=.*[[:alnum:]_]+\.[[:alnum:]_]+=' "$TMP" >&2
  exit 1
fi

echo "PASS: $IMG build identity properties are present and well formed"

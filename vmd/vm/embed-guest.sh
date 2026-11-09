#!/usr/bin/env bash
# Append a guest set to a vmd binary so it boots with no -img.
#
#   vm/embed-guest.sh <vmd-binary> <guest-set-dir|guest.tar.*>
#
# Layout appended to the file: <tar><u64 little-endian size><"HIBGUEST">.
# vmd probes its own tail for the magic, extracts the tar and boots the
# image set inside.
set -euo pipefail

BIN="$1"
SRC="$2"
[ -f "$BIN" ] || { echo "embed-guest: no binary $BIN" >&2; exit 1; }

if [ -d "$SRC" ]; then
	[ -f "$SRC/guest.env" ] || {
		echo "embed-guest: $SRC has no guest.env" >&2; exit 1; }
	TMP=$(mktemp --suffix=.tar.xz)
	trap 'rm -f "$TMP"' EXIT
	tar -cJf "$TMP" -C "$SRC" .
	SRC="$TMP"
elif [ ! -f "$SRC" ]; then
	echo "embed-guest: no guest set $SRC" >&2; exit 1
fi

sz=$(stat -c%s "$SRC")
cat "$SRC" >> "$BIN"
for i in 0 1 2 3 4 5 6 7; do
	printf "\\$(printf '%03o' $(( sz >> (8*i) & 255 )))" >> "$BIN"
done
printf 'HIBGUEST' >> "$BIN"
echo "embed-guest: $BIN carries a $sz-byte guest bundle"

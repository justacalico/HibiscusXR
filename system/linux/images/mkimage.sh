#!/bin/bash
# Assemble a Hibiscus Linux image for ARCH.
#
#   images/mkimage.sh -a x86_64 -o out.img [-b base.tar.xz] [-d profile]
#
# The base rootfs comes from -b (any distro tarball: Alpine, Debian,
# Fedora - anything with systemd); without one, a staging tree with
# just our binaries, units and profiles is produced, which is what CI
# and the test suite check.
set -e
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)

ARCH=""
OUT=""
BASE=""
PROFILE="generic"
while [ $# -gt 0 ]; do
	case "$1" in
	-a) ARCH="$2"; shift 2 ;;
	-o) OUT="$2"; shift 2 ;;
	-b) BASE="$2"; shift 2 ;;
	-d) PROFILE="$2"; shift 2 ;;
	*) echo "unknown arg: $1" >&2; exit 2 ;;
	esac
done
[ -n "$ARCH" ] || { echo "need -a <arch>" >&2; exit 2; }
[ -n "$OUT" ] || { echo "need -o <out>" >&2; exit 2; }

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT

# 1. base rootfs
if [ -n "$BASE" ]; then
	case "$BASE" in
	*.tar.xz) tar -xJf "$BASE" -C "$STAGE" ;;
	*.tar.gz|*.tgz) tar -xzf "$BASE" -C "$STAGE" ;;
	*.tar) tar -xf "$BASE" -C "$STAGE" ;;
	*) echo "mkimage: unsupported base format: $BASE" >&2; exit 1 ;;
	esac
fi

# 2. our userspace
ARCH="$ARCH" make -C "$ROOT/system/linux" -s
make -C "$ROOT/system/linux" -s install DESTDIR="$STAGE" PREFIX=/usr ARCH="$ARCH"

# 3. device profile -> /etc/hibiscus/device.env + wanted units
install -d "$STAGE/etc/hibiscus"
prof="$ROOT/system/linux/devices/$PROFILE.env"
[ -f "$prof" ] || { echo "mkimage: no profile $PROFILE" >&2; exit 1; }
cp "$prof" "$STAGE/etc/hibiscus/device.env"
. "$prof"
wants="$STAGE/etc/systemd/system/multi-user.target.wants"
mkdir -p "$wants" "$STAGE/etc/hibiscus/svc"
ln -sf "/usr/lib/systemd/system/hsvcd.service" "$wants/hsvcd.service"
for svc in $HSVC_SVC_NAMES; do
	var="HSVC_$(echo "$svc" | tr a-z A-Z)_BIN"
	bin="${!var:?mkimage: profile missing $var}"
	var="HSVC_$(echo "$svc" | tr a-z A-Z)_ARGS"
	printf 'HSVC_BIN=%s\nHSVC_ARGS="%s"\n' "$bin" "${!var}" \
		> "$STAGE/etc/hibiscus/svc/$svc.env"
	ln -sf "/usr/lib/systemd/system/hsvc-svc@.service" \
		"$wants/hsvc-svc@$svc.service"
done

# 4. arch marker + release file
echo "hibiscus-linux-$ARCH-$PROFILE" > "$STAGE/etc/hibiscus-release"

# 5. output
mkdir -p "$(dirname "$OUT")"
tar -cf "$OUT" -C "$STAGE" .
echo "mkimage: $OUT ($ARCH/$PROFILE)"

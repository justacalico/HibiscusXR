#!/bin/bash
# Assemble a Hibiscus Linux image for ARCH.
#
#   images/mkimage.sh -a x86_64 -o out.img [-b base.tar.xz] [-d profile]
#                    [-x xr-dir]
#
# For a foreign ARCH on a host with a bare cross gcc, SYSROOT=<dir> in
# the environment feeds the cross build (images/fetch-sysroot.sh makes
# one; --shell adds the dep set the shell's cross build wants).
#
# The base rootfs comes from -b (any distro tarball: Alpine, Debian,
# Fedora - anything with systemd); without one, a staging tree with
# just our binaries, units and profiles is produced, which is what CI
# and the test suite check. -x points at the OpenXR runtime + shell
# artifacts (xr/build-runtime.sh output plus a dash-wayland build);
# profiles that set HSVC_XR=1 get monado.service and
# hibiscus-shell.service enabled when the artifacts are staged.
set -e
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)

ARCH=""
OUT=""
BASE=""
XR=""
PROFILE="generic"
while [ $# -gt 0 ]; do
	case "$1" in
	-a) ARCH="$2"; shift 2 ;;
	-o) OUT="$2"; shift 2 ;;
	-b) BASE="$2"; shift 2 ;;
	-d) PROFILE="$2"; shift 2 ;;
	-x) XR="$2"; shift 2 ;;
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
ARCH="$ARCH" SYSROOT="${SYSROOT:-}" make -C "$ROOT/system/linux" -s
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

# 3b. OpenXR runtime + shell, when the profile wants them and the
# artifacts were built. The runtime is the piece that turns the bus
# into OpenXR devices - monado-service plus the hsvc driver in
# libopenxr_monado.so - and the shell is the dash's own wayland
# compositor presenting through XR_EXTX_overlay. Every staged ELF is
# checked against -a so a host-arch artifact can never slip into a
# foreign-arch image.
XR="${XR:-$ROOT/system/linux/out/xr}"
want_mach="X86-64"
[ "$ARCH" = aarch64 ] && want_mach="AArch64"
is_arch() {
	readelf -h "$1" 2>/dev/null | grep -q "Machine:.*$want_mach"
}
if [ "${HSVC_XR:-0}" = 1 ]; then
	rt="$XR/lib/libopenxr_monado.so"
	if [ -f "$rt" ] && is_arch "$rt" && is_arch "$XR/bin/monado-service"; then
		install -d "$STAGE/usr/lib" "$STAGE/usr/bin" \
			"$STAGE/etc/openxr/1"
		install -m755 "$rt" "$STAGE/usr/lib/"
		install -m755 "$XR/bin/monado-service" "$STAGE/usr/bin/"
		install -m755 "$XR/bin/monado-cli" "$STAGE/usr/bin/" 2>/dev/null || true
		install -m644 "$XR/share/openxr/1/active_runtime.json" \
			"$STAGE/etc/openxr/1/"
		ln -sf "/usr/lib/systemd/system/monado.service" \
			"$wants/monado.service"
		if [ -f "$XR/bin/dash-wayland" ]; then
			if is_arch "$XR/bin/dash-wayland"; then
				install -m755 "$XR/bin/dash-wayland" "$STAGE/usr/bin/"
				ln -sf "/usr/lib/systemd/system/hibiscus-shell.service" \
					"$wants/hibiscus-shell.service"
			else
				echo "mkimage: dash-wayland is not an $ARCH build, shell not enabled" >&2
			fi
		else
			echo "mkimage: no dash-wayland in $XR/bin, shell not enabled" >&2
		fi
	elif [ -f "$rt" ]; then
		echo "mkimage: $XR runtime is not an $ARCH build, xr not staged" >&2
	else
		echo "mkimage: profile wants xr but $XR has no runtime" >&2
	fi
fi

# 4. arch marker + release file
echo "hibiscus-linux-$ARCH-$PROFILE" > "$STAGE/etc/hibiscus-release"

# 5. output
mkdir -p "$(dirname "$OUT")"
tar -cf "$OUT" -C "$STAGE" .
echo "mkimage: $OUT ($ARCH/$PROFILE)"

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
ln -sf "/usr/lib/systemd/system/hibiscus-user.service" \
	"$wants/hibiscus-user.service"
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
# the runtime + shell for ARCH live in out/xr (host arch) or
# out/xr-<arch> (cross) - build-runtime.sh and `make shell` both land
# there; -x overrides
if [ -z "$XR" ]; then
	XR="$ROOT/system/linux/out/xr"
	[ "$ARCH" = "$(uname -m)" ] || XR="$ROOT/system/linux/out/xr-$ARCH"
fi
want_mach="X86-64"
[ "$ARCH" = aarch64 ] && want_mach="AArch64"
is_arch() {
	readelf -h "$1" 2>/dev/null | grep -q "Machine:.*$want_mach"
}

# Vendoring: the staged xr binaries' shared-lib closure lands in
# /usr/lib/hibiscus and their rpaths point there, so the shell and the
# runtime run on a base that never shipped the deps. The glibc
# baseline, the dynamic loader and the openxr names (mkimage aliases
# them to the monado library) stay unvendored - anything running
# systemd carries the rest
core_dep() {
	case "$1" in
	libc.so.*|libm.so.*|libdl.so.*|librt.so.*|libpthread.so.*| \
	libresolv.so.*|ld-linux*|linux-vdso*) return 0 ;;
	esac
	return 1
}
elf_deps() {
	readelf -d "$1" 2>/dev/null | \
		sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p'
}
VDIR=""
VMISS=""
find_lib() {
	local d
	for d in "${VLIBDIRS[@]}"; do
		[ -e "$d/$1" ] && { echo "$d/$1"; return 0; }
	done
	return 1
}
vendor_walk() {
	local n p
	for n in $(elf_deps "$1"); do
		core_dep "$n" && continue
		case " $VDONE " in *" $n "*) continue ;; esac
		VDONE="$VDONE$n "
		if p=$(find_lib "$n"); then
			mkdir -p "$VDIR"
			cp -L "$p" "$VDIR/$n"
			vendor_walk "$p"
		else
			VMISS="$VMISS $n"
		fi
	done
}
vendor_elf() {
	# host arch resolves through ldd (the real loader); a foreign arch
	# walks DT_NEEDED against the SYSROOT lib dirs recursively
	if [ "$ARCH" = "$(uname -m)" ]; then
		local n arrow p miss
		ldd "$1" 2>/dev/null | while read -r n arrow p _; do
			[ "$arrow" = "=>" ] || continue
			case "$p" in /*) ;; *) continue ;; esac
			core_dep "$n" && continue
			# a dep vendored for an earlier elf resolves back into
			# $VDIR through the binary's own rpath - cp would fail on
			# same-file, it is already where it needs to be
			[ "$p" -ef "$VDIR/$n" ] && continue
			mkdir -p "$VDIR" && cp -L "$p" "$VDIR/$n"
		done
		miss=$(ldd "$1" 2>/dev/null | \
			awk '/not found/{c++} END{print c+0}')
		[ "$miss" != 0 ] && VMISS="$VMISS $1($miss missing)"
		return 0
	fi
	vendor_walk "$1"
	return 0
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
		VDIR="$STAGE/usr/lib/hibiscus"
		VLIBDIRS=()
		VSHAREDIRS=()
		if [ "$ARCH" != "$(uname -m)" ]; then
			for d in "$SYSROOT/usr/lib64" "$SYSROOT/lib64" \
					"$SYSROOT/usr/lib" "$SYSROOT/lib" \
					"$SYSROOT/usr/lib/$ARCH-linux-gnu"; do
				[ -d "$d" ] && VLIBDIRS+=("$d")
			done
			for d in "$SYSROOT/usr/share" "$SYSROOT/usr/local/share"; do
				[ -d "$d" ] && VSHAREDIRS+=("$d")
			done
			[ ${#VLIBDIRS[@]} -eq 0 ] && echo \
				"mkimage: no sysroot lib dirs, xr deps not vendored" >&2
		else
			for d in /usr/lib64 /usr/lib /lib64 \
					/usr/lib/"$(uname -m)"-linux-gnu; do
				[ -d "$d" ] && VLIBDIRS+=("$d")
			done
			for d in /usr/share /usr/local/share; do
				[ -d "$d" ] && VSHAREDIRS+=("$d")
			done
		fi
		VDONE=" "
		for elf in "$STAGE/usr/bin/dash-wayland" \
				"$STAGE/usr/bin/monado-service" \
				"$STAGE/usr/bin/monado-cli" \
				"$STAGE/usr/lib/libopenxr_monado.so"; do
			if [ -f "$elf" ]; then vendor_elf "$elf"; fi
		done
		mkdir -p "$VDIR"
		# openxr clients (the dash links libopenxr_loader.so.1) resolve
		# through the vendored Khronos loader - the runtime library only
		# exports xrNegotiateLoaderRuntimeInterface, aliasing it under
		# the loader names left clients with unresolved xr* symbols.
		# /usr/lib is not in every base's default search path (Fedora
		# looks at lib64), so the loader names get aliases in both
		# places pointing at the vendored loader
		if [ -f "$VDIR/libopenxr_loader.so.1" ]; then
			ln -sf "hibiscus/libopenxr_loader.so.1" \
				"$STAGE/usr/lib/libopenxr_loader.so.1"
			ln -sf "hibiscus/libopenxr_loader.so.1" \
				"$STAGE/usr/lib/libopenxr.so.1"
			ln -sf "libopenxr_loader.so.1" "$VDIR/libopenxr.so.1"
		else
			echo "mkimage: no libopenxr_loader.so.1 vendored," \
				"openxr clients cannot link" >&2
		fi

		# Drivers the DT_NEEDED walk cannot see: the Vulkan loader finds
		# ICDs by json, mesa EGL its DRI driver by dir, glvnd its vendor
		# lib by json. Stage the software set (lavapipe for the
		# compositor, swrast for the shell's GLES) off the host libdirs
		# or SYSROOT when cross, so a base without GL still runs the
		# shell; a base or profile that ships real drivers keeps them,
		# the loaders just see one more ICD.
		find_in() {
			local p d
			for p in "$@"; do
				for d in "${VLIBDIRS[@]}"; do
					[ -e "$d/$p" ] && { echo "$d/$p"; return 0; }
				done
			done
			return 0
		}
		find_data() {
			local p d
			for p in "$@"; do
				for d in "${VSHAREDIRS[@]}"; do
					[ -e "$d/$p" ] && { echo "$d/$p"; return 0; }
				done
			done
			return 0
		}
		vendor_file() {
			local dst="$VDIR/$2"
			mkdir -p "$(dirname "$dst")"
			cp -L "$1" "$dst"
			vendor_elf "$dst"
		}

		icd=$(find_data "vulkan/icd.d/lvp_icd.$ARCH.json" \
				"vulkan/icd.d/lvp_icd.json")
		if [ -n "$icd" ]; then
			lvp_name=$(sed -n \
				's/.*"library_path"[^"]*"\([^"]*\)".*/\1/p' \
				"$icd" | head -1)
			lvp_name="${lvp_name##*/}"
			lvp_src=$(find_in "dri-freeworld/$lvp_name" "$lvp_name")
			if [ -n "$lvp_src" ]; then
				vendor_file "$lvp_src" "$lvp_name"
				install -d "$STAGE/etc/vulkan/icd.d"
				sed 's|"library_path"[^,}]*|"library_path": "/usr/lib/hibiscus/'"$lvp_name"'"|' \
					"$icd" > \
					"$STAGE/etc/vulkan/icd.d/lvp_icd.$ARCH.json"
			else
				echo "mkimage: lvp icd names $lvp_name, not found" >&2
			fi
		else
			echo "mkimage: no lavapipe icd found," \
				"compositor has no vulkan device" >&2
		fi

		swr=$(find_in "dri/swrast_dri.so")
		if [ -n "$swr" ]; then
			mkdir -p "$VDIR/dri"
			if [ -L "$swr" ]; then
				dril=$(basename "$(readlink -f "$swr")")
				vendor_file "$(readlink -f "$swr")" "dri/$dril"
				ln -sf "$dril" "$VDIR/dri/swrast_dri.so"
			else
				vendor_file "$swr" "dri/swrast_dri.so"
			fi
		else
			echo "mkimage: no swrast dri driver found," \
				"shell GLES may not run" >&2
		fi

		eglmesa=$(find_in "libEGL_mesa.so.0")
		if [ -n "$eglmesa" ]; then
			vendor_file "$eglmesa" "libEGL_mesa.so.0"
			egljson=$(find_data "glvnd/egl_vendor.d/50_mesa.json")
			if [ -n "$egljson" ]; then
				install -d "$STAGE/usr/share/glvnd/egl_vendor.d"
				sed 's|"library_path"[^,}]*|"library_path" : "/usr/lib/hibiscus/libEGL_mesa.so.0"|' \
					"$egljson" > \
					"$STAGE/usr/share/glvnd/egl_vendor.d/50_mesa.json"
			fi
		else
			echo "mkimage: no libEGL_mesa.so.0 found," \
				"shell GLES may not run" >&2
		fi

		# the vendored dir on the dynamic linker's map: dlopened vendor
		# and driver libs pull their own deps out of it (Fedora's
		# ldconfig.service rebuilds the cache at boot)
		install -d "$STAGE/etc/ld.so.conf.d"
		echo "/usr/lib/hibiscus" > "$STAGE/etc/ld.so.conf.d/hibiscus.conf"

		if [ -n "$VMISS" ]; then
			echo "mkimage: deps left to the base:$VMISS" >&2
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
tar -cf "$OUT" --numeric-owner --owner=0 --group=0 -C "$STAGE" .
echo "mkimage: $OUT ($ARCH/$PROFILE)"

#!/bin/bash
# Fetch a minimal aarch64 glibc sysroot for cross-building (and as a
# starting mkimage -b base). Fedora Everything RPMs, pinned below;
# bump the pins when moving Fedora releases.
#
#   images/fetch-sysroot.sh <dir>          libc only - enough for make
#   images/fetch-sysroot.sh <dir> --shell  plus the dash-wayland dep set:
#                                         wayland/xkbcommon/libpng/drm/
#                                         glvnd/glfw/openxr/zlib + c++
#                            --vmd        plus the vmd host driver dep set:
#                                         epoxy + the X11 link chain;
#                                         implies --shell for glfw/openxr
#                            --runtime     plus the monado dep set on top
#                                         (udev/systemd/dbus/vulkan);
#                                         implies --shell for wayland+drm
set -e
DIR="$1"
[ -n "$DIR" ] || { echo "usage: fetch-sysroot.sh <dir> [--shell] [--vmd] [--runtime]" >&2; exit 2; }
shift
mkdir -p "$DIR/root"

BASE="https://dl.fedoraproject.org/pub/fedora/linux/releases/44/Everything/aarch64/os/Packages"
PKGS="
g/glibc-2.43-2.fc44.aarch64.rpm
g/glibc-devel-2.43-2.fc44.aarch64.rpm
g/glibc-static-2.43-2.fc44.aarch64.rpm
k/kernel-headers-6.19.6-300.fc44.aarch64.rpm
l/libatomic-16.0.1-0.10.fc44.aarch64.rpm
"

SHELL_SET=0
VMD_SET=0
RT_SET=0
for a in "$@"; do
	case "$a" in
	--shell) SHELL_SET=1 ;;
	--vmd) SHELL_SET=1; VMD_SET=1 ;;
	--runtime) SHELL_SET=1; RT_SET=1 ;;
	*) echo "unknown flag: $a" >&2; exit 2 ;;
	esac
done

if [ "$SHELL_SET" = 1 ]; then
	PKGS="$PKGS
l/libgcc-16.0.1-0.10.fc44.aarch64.rpm
l/libstdc++-16.0.1-0.10.fc44.aarch64.rpm
l/libstdc++-devel-16.0.1-0.10.fc44.aarch64.rpm
l/libffi-3.5.2-2.fc44.aarch64.rpm
l/libffi-devel-3.5.2-2.fc44.aarch64.rpm
l/libwayland-server-1.24.0-3.fc44.aarch64.rpm
l/libwayland-client-1.24.0-3.fc44.aarch64.rpm
l/libwayland-egl-1.24.0-3.fc44.aarch64.rpm
l/libwayland-cursor-1.24.0-3.fc44.aarch64.rpm
w/wayland-devel-1.24.0-3.fc44.aarch64.rpm
w/wayland-protocols-devel-1.47-2.fc44.noarch.rpm
l/libxkbcommon-1.13.1-2.fc44.aarch64.rpm
l/libxkbcommon-devel-1.13.1-2.fc44.aarch64.rpm
l/libpng-1.6.55-1.fc44.aarch64.rpm
l/libpng-devel-1.6.55-1.fc44.aarch64.rpm
l/libdrm-2.4.131-1.fc44.aarch64.rpm
l/libdrm-devel-2.4.131-1.fc44.aarch64.rpm
l/libglvnd-1.7.0-9.fc44.aarch64.rpm
l/libglvnd-egl-1.7.0-9.fc44.aarch64.rpm
l/libglvnd-gles-1.7.0-9.fc44.aarch64.rpm
l/libglvnd-devel-1.7.0-9.fc44.aarch64.rpm
g/glfw-3.4-6.fc44.aarch64.rpm
g/glfw-devel-3.4-6.fc44.aarch64.rpm
j/jsoncpp-1.9.6-3.fc44.aarch64.rpm
o/openxr-1.1.57-1.fc44.aarch64.rpm
o/openxr-libs-1.1.57-1.fc44.aarch64.rpm
o/openxr-devel-1.1.57-1.fc44.aarch64.rpm
z/zlib-ng-compat-2.3.3-3.fc44.aarch64.rpm
z/zlib-ng-compat-devel-2.3.3-3.fc44.aarch64.rpm
"
fi

if [ "$VMD_SET" = 1 ]; then
	PKGS="$PKGS
l/libepoxy-1.5.10-12.fc44.aarch64.rpm
l/libepoxy-devel-1.5.10-12.fc44.aarch64.rpm
l/libXau-1.0.12-4.fc44.aarch64.rpm
l/libXau-devel-1.0.12-4.fc44.aarch64.rpm
l/libXdmcp-1.1.5-5.fc44.aarch64.rpm
l/libXdmcp-devel-1.1.5-5.fc44.aarch64.rpm
l/libxcb-1.17.0-7.fc44.aarch64.rpm
l/libxcb-devel-1.17.0-7.fc44.aarch64.rpm
l/libX11-1.8.13-1.fc44.aarch64.rpm
l/libX11-devel-1.8.13-1.fc44.aarch64.rpm
l/libXext-1.3.6-5.fc44.aarch64.rpm
l/libXext-devel-1.3.6-5.fc44.aarch64.rpm
l/libXrender-0.9.12-4.fc44.aarch64.rpm
l/libXrender-devel-0.9.12-4.fc44.aarch64.rpm
l/libXrandr-1.5.4-7.fc44.aarch64.rpm
l/libXrandr-devel-1.5.4-7.fc44.aarch64.rpm
x/xorg-x11-proto-devel-2025.1-2.fc44.noarch.rpm
"
fi

if [ "$RT_SET" = 1 ]; then
	PKGS="$PKGS
s/systemd-libs-259.5-1.fc44.aarch64.rpm
s/systemd-devel-259.5-1.fc44.aarch64.rpm
d/dbus-libs-1.16.2-1.fc44.aarch64.rpm
d/dbus-devel-1.16.2-1.fc44.aarch64.rpm
v/vulkan-headers-1.4.341.0-1.fc44.noarch.rpm
v/vulkan-loader-1.4.341.0-1.fc44.aarch64.rpm
v/vulkan-loader-devel-1.4.341.0-1.fc44.aarch64.rpm
l/libXau-1.0.12-4.fc44.aarch64.rpm
l/libXau-devel-1.0.12-4.fc44.aarch64.rpm
l/libXdmcp-1.1.5-5.fc44.aarch64.rpm
l/libXdmcp-devel-1.1.5-5.fc44.aarch64.rpm
l/libglvnd-opengl-1.7.0-9.fc44.aarch64.rpm
l/libglvnd-glx-1.7.0-9.fc44.aarch64.rpm
l/libxcb-1.17.0-7.fc44.aarch64.rpm
l/libxcb-devel-1.17.0-7.fc44.aarch64.rpm
l/libX11-1.8.13-1.fc44.aarch64.rpm
l/libX11-xcb-1.8.13-1.fc44.aarch64.rpm
l/libX11-devel-1.8.13-1.fc44.aarch64.rpm
l/libXext-1.3.6-5.fc44.aarch64.rpm
l/libXext-devel-1.3.6-5.fc44.aarch64.rpm
l/libXrender-0.9.12-4.fc44.aarch64.rpm
l/libXrender-devel-0.9.12-4.fc44.aarch64.rpm
l/libXrandr-1.5.4-7.fc44.aarch64.rpm
l/libXrandr-devel-1.5.4-7.fc44.aarch64.rpm
x/xorg-x11-proto-devel-2025.1-2.fc44.noarch.rpm
"
fi

# usrmerge layout: ld expects /lib and /lib64 to exist - make the links
# before extracting so rpms that ship under /lib64 (libgcc) land in
# usr/lib64 through them
ln -sfn usr/lib64 "$DIR/root/lib64"
ln -sfn usr/lib "$DIR/root/lib"

for p in $PKGS; do
	f="$DIR/$(basename "$p")"
	[ -f "$f" ] || curl -sfL -o "$f" "$BASE/$p"
done
for f in "$DIR"/*.rpm; do
	rpm2cpio "$f" | (cd "$DIR/root" && cpio -idm --quiet)
done

# Fedora ships libgcc_s and libstdc++ under soname/gcc-dir paths a bare
# cross toolchain never searches; relink the -l names into usr/lib64
if [ -f "$DIR/root/usr/lib64/libgcc_s.so.1" ]; then
	ln -sfn libgcc_s.so.1 "$DIR/root/usr/lib64/libgcc_s.so"
fi
for s in "$DIR"/root/usr/lib/gcc/*/*/libstdc++.so; do
	[ -e "$s" ] || continue
	ln -sfn "../lib/gcc/${s#"$DIR"/root/usr/lib/gcc/}" \
		"$DIR/root/usr/lib64/libstdc++.so"
done
# Fedora gcc specs auto-add -latomic_asneeded; the script ships in the
# native gcc package so the cross sysroot has to provide it. Static
# links look for the .a name instead - a bare script keeps the specs
# satisfied without dragging in a static libatomic nothing needs
cat > "$DIR/root/usr/lib64/libatomic_asneeded.so" <<'LD'
OUTPUT_FORMAT(elf64-littleaarch64)
INPUT ( AS_NEEDED ( -latomic ) )
LD
cat > "$DIR/root/usr/lib64/libatomic_asneeded.a" <<'LD'
OUTPUT_FORMAT(elf64-littleaarch64)
LD
ln -sfn libatomic.so.1.2.0 "$DIR/root/usr/lib64/libatomic.so"

# wayland-scanner in the sysroot is an aarch64 binary, but cmake runs it
# as a host tool during the runtime's cross build - the generated code
# is arch-neutral, so point it at the host's scanner when one exists
if [ -f "$DIR/root/usr/bin/wayland-scanner" ] && \
	command -v wayland-scanner >/dev/null; then
	ln -sfn "$(command -v wayland-scanner)" \
		"$DIR/root/usr/bin/wayland-scanner"
fi

echo "sysroot: $DIR/root"
echo "use: make ARCH=aarch64 SYSROOT=$DIR/root"
echo "     make dist A64SYSROOT=$DIR/root"
echo "     make shell ARCH=aarch64 SYSROOT=$DIR/root     (needs --shell fetch)"
echo "     make -C vmd ARCH=aarch64 SYSROOT=$DIR/root     (needs --vmd fetch)"
echo "     ARCH=aarch64 SYSROOT=$DIR/root xr/build-runtime.sh   (needs --runtime fetch)"

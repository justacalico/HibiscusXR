#!/bin/bash
# Fetch a minimal aarch64 glibc sysroot for cross-building (and as a
# starting mkimage -b base). Fedora Everything RPMs, pinned below;
# bump the pins when moving Fedora releases.
#
#   images/fetch-sysroot.sh <dir>          libc only - enough for make
#   images/fetch-sysroot.sh <dir> --shell  plus the dash-wayland dep set:
#                                         wayland/xkbcommon/libpng/drm/
#                                         glvnd/glfw/openxr/zlib + c++
set -e
DIR="$1"
[ -n "$DIR" ] || { echo "usage: fetch-sysroot.sh <dir> [--shell]" >&2; exit 2; }
mkdir -p "$DIR/root"

BASE="https://dl.fedoraproject.org/pub/fedora/linux/releases/44/Everything/aarch64/os/Packages"
PKGS="
g/glibc-2.43-2.fc44.aarch64.rpm
g/glibc-devel-2.43-2.fc44.aarch64.rpm
g/glibc-static-2.43-2.fc44.aarch64.rpm
k/kernel-headers-6.19.6-300.fc44.aarch64.rpm
l/libatomic-16.0.1-0.10.fc44.aarch64.rpm
"

if [ "$2" = "--shell" ]; then
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

echo "sysroot: $DIR/root"
echo "use: make ARCH=aarch64 SYSROOT=$DIR/root"
echo "     make dist A64SYSROOT=$DIR/root"
echo "     make shell ARCH=aarch64 SYSROOT=$DIR/root   (needs --shell fetch)"

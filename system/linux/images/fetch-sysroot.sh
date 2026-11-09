#!/bin/bash
# Fetch a minimal aarch64 glibc sysroot for cross-building (and as a
# starting mkimage -b base). Fedora Everything RPMs, pinned below;
# bump the pins when moving Fedora releases.
#
#   images/fetch-sysroot.sh <dir>
set -e
DIR="$1"
[ -n "$DIR" ] || { echo "usage: fetch-sysroot.sh <dir>" >&2; exit 2; }
mkdir -p "$DIR/root"

BASE="https://dl.fedoraproject.org/pub/fedora/linux/releases/44/Everything/aarch64/os/Packages"
PKGS="
g/glibc-2.43-2.fc44.aarch64.rpm
g/glibc-devel-2.43-2.fc44.aarch64.rpm
k/kernel-headers-6.19.6-300.fc44.aarch64.rpm
l/libatomic-16.0.1-0.10.fc44.aarch64.rpm
"

for p in $PKGS; do
	f="$DIR/$(basename "$p")"
	[ -f "$f" ] || curl -sfL -o "$f" "$BASE/$p"
done
for f in "$DIR"/*.rpm; do
	rpm2cpio "$f" | (cd "$DIR/root" && cpio -idm --quiet)
done

# usrmerge layout: ld expects /lib and /lib64 to exist
ln -sfn usr/lib64 "$DIR/root/lib64"
ln -sfn usr/lib "$DIR/root/lib"
# Fedora gcc specs auto-add -latomic_asneeded; the script ships in the
# native gcc package so the cross sysroot has to provide it
cat > "$DIR/root/usr/lib64/libatomic_asneeded.so" <<'LD'
OUTPUT_FORMAT(elf64-littleaarch64)
INPUT ( AS_NEEDED ( -latomic ) )
LD
ln -sfn libatomic.so.1.2.0 "$DIR/root/usr/lib64/libatomic.so"

echo "sysroot: $DIR/root"
echo "use: make ARCH=aarch64 SYSROOT=$DIR/root"

#!/usr/bin/env bash
# Build the Hibiscus Linux guest set vmd boots in qemu: a virtio-capable
# kernel, a busybox initramfs that loads the disk modules and switches
# root, and a real ext4 rootfs - a Fedora userspace with the distro's
# own binaries, units and device profile staged on top.
#
#   vm/make-guest.sh [-a arch] [-o dir] [-d profile] [-b base.tar]
#
#   -a   guest arch: x86_64 (default: host) or aarch64
#   -o   output dir (default: vmd/out/guest-<arch>)
#   -d   device profile staged by mkimage (default: generic)
#   -b   base rootfs tarball to layer on instead of fetching Fedora;
#        anything mkimage -b accepts, must already contain systemd
#
# Downloads resolve the current version from the source's own index
# (Alpine APKINDEX, Fedora releases.json - the base is sha256-verified),
# so no version pin rots in the tree.
#
# Writes into the output dir:
#   vmlinuz        kernel (Alpine linux-virt for the guest arch)
#   initramfs.img  cpio.gz: busybox + virtio_blk/ext4 module set
#   rootfs.img     ext4: Fedora base + system/linux staging tree
#   guest.env      manifest vmd reads: arch, machine, cpu, qemu, cmdline
set -euo pipefail

# Image assembly runs as fake root in a user namespace: the rootfs has
# 000-mode files and suid binaries a plain user cannot extract or read
# back, and mkfs.ext4 -d needs them owned by uid 0 in the image.
if [ "$(id -u)" != 0 ] && [ -z "${VMD_GUEST_ROOT:-}" ]; then
	exec unshare -r env VMD_GUEST_ROOT=1 bash "$0" "$@"
fi
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)

GARCH=$(uname -m)
OUT=""
BASE=""
PROFILE="generic"
while [ $# -gt 0 ]; do
	case "$1" in
	-a) GARCH="$2"; shift 2 ;;
	-o) OUT="$2"; shift 2 ;;
	-d) PROFILE="$2"; shift 2 ;;
	-b) BASE="$2"; shift 2 ;;
	*) echo "unknown arg: $1" >&2; exit 2 ;;
	esac
done
case "$GARCH" in
x86_64|aarch64) ;;
*) echo "make-guest: unsupported arch $GARCH" >&2; exit 2 ;;
esac
OUT="${OUT:-$PWD/out/guest-$GARCH}"
mkdir -p "$OUT"
WORK=$(mktemp -d)
trap 'chmod -R u+rwX "$WORK" 2>/dev/null || true; rm -rf "$WORK"' EXIT

ALPINE="https://dl-cdn.alpinelinux.org/alpine/v3.20/main/$GARCH"
FEDORA_REL="44"
# downloads land here so a rebuild doesn't refetch a few hundred MB
CACHE="${VMD_GUEST_CACHE:-$PWD/out/cache/$GARCH}"
mkdir -p "$CACHE"

# fetch_apk <pkg> <dest.apk> - resolve <pkg>'s current version from the
# Alpine APKINDEX and download it. (The index's C: field cannot verify
# the file - it predates re-signing - so HTTPS + a clean tar extract is
# the integrity check, same as the Fedora RPM fetches.)
fetch_apk() {
	local pkg="$1" dest="$2" index="$WORK/APKINDEX"
	if [ ! -f "$index" ]; then
		curl -sfL --retry 3 "$ALPINE/APKINDEX.tar.gz" | tar -xzOf - APKINDEX > "$index"
	fi
	local ver
	ver=$(awk -v p="$1" '/^P:/{n=substr($0,3)}
	      n==p && /^V:/{print substr($0,3); exit}' "$index")
	[ -n "$ver" ] || { echo "make-guest: $1 not in APKINDEX" >&2; exit 1; }
	# cache by versioned name: a new upstream release fetches fresh,
	# a rerun reuses the byte-identical apk
	local cached="$CACHE/$1-$ver.apk"
	[ -f "$cached" ] || {
		echo "make-guest: fetch $1-$ver" >&2
		curl -sfL --retry 3 -o "$cached" "$ALPINE/$1-$ver.apk"
	}
	cp "$cached" "$dest"
	echo "$ver"
}

# --- kernel: Alpine linux-virt ----------------------------------------
# virt kernel: virtio-pci built in, virtio_blk/ext4 as modules - the
# initramfs carries the mount set, the rootfs gets the whole tree.
KVER=$(fetch_apk linux-virt "$WORK/linux-virt.apk")
mkdir -p "$WORK/kv"
tar -xzf "$WORK/linux-virt.apk" -C "$WORK/kv" boot lib/modules 2>/dev/null
KIMG=$(find "$WORK/kv/boot" -name 'vmlinuz-*' | head -1)
KREL=$(find "$WORK/kv/lib/modules" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | head -1)
[ -n "$KIMG" ] && [ -n "$KREL" ] || { echo "make-guest: kernel apk missing files" >&2; exit 1; }
cp "$KIMG" "$OUT/vmlinuz"

# --- busybox for the initramfs -----------------------------------------
fetch_apk busybox-static "$WORK/busybox.apk" >/dev/null
mkdir -p "$WORK/bb"
tar -xzf "$WORK/busybox.apk" -C "$WORK/bb" bin/busybox.static 2>/dev/null

# --- initramfs ----------------------------------------------------------
RD="$WORK/rd"
mkdir -p "$RD/bin" "$RD/sbin" "$RD/proc" "$RD/sys" "$RD/dev" "$RD/newroot"
install -m755 "$WORK/bb/bin/busybox.static" "$RD/bin/busybox"
ln -s /bin/busybox "$RD/sbin/modprobe"
MODD="$RD/lib/modules/$KREL"
mkdir -p "$MODD"
# mount chain: virtio_blk + the ext4 metadata deps the crypto request
# pulls in (crc32c is a module, so modprobe has to load it by alias
# before mount or the fs open fails - /sbin/modprobe symlink above
# covers kernel auto-requests too)
for m in lib/crc16 lib/libcrc32c crypto/crc32c_generic crypto/crc32c-intel \
	crypto/crc32c-aarch64 fs/mbcache fs/jbd2/jbd2 fs/ext4/ext4 \
	drivers/block/virtio_blk; do
	src="$WORK/kv/lib/modules/$KREL/kernel/$m.ko.gz"
	[ -f "$src" ] || continue
	mkdir -p "$MODD/kernel/$(dirname "$m")"
	zcat "$src" > "$MODD/kernel/$m.ko"
done
depmod -b "$RD" "$KREL" 2>/dev/null || depmod -a -b "$RD" "$KREL"
cat > "$RD/init" <<'INIT'
#!/bin/busybox sh
export PATH=/bin:/sbin
/bin/busybox --install -s /bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null

modprobe virtio_blk
modprobe crc32c 2>/dev/null || true
modprobe ext4
n=0
while [ ! -b /dev/vda ] && [ $n -lt 200 ]; do n=$((n+1)); sleep 0.1; done
if [ ! -b /dev/vda ]; then
	echo "initramfs: no root disk" >&2
	exec sh
fi
mount -t ext4 -o rw /dev/vda /newroot || exec sh
if [ -e /newroot/sbin/init ]; then
	exec switch_root /newroot /sbin/init
fi
echo "initramfs: no /sbin/init on root disk" >&2
exec sh
INIT
chmod +x "$RD/init"
( cd "$RD" && find . | cpio -o -H newc --quiet | gzip -9 ) > "$OUT/initramfs.img"

# --- base rootfs ---------------------------------------------------------
if [ -z "$BASE" ]; then
	# Fedora's WSL rootfs: a full userspace (systemd, dnf, coreutils,
	# udev) as a plain tar.gz - the container base ships no init.
	link=$(curl -sfL --retry 3 https://fedoraproject.org/releases.json |
		python3 -c "
import json,sys
for e in json.load(sys.stdin):
    if e.get('version')=='$FEDORA_REL' and e.get('variant')=='Container' \
       and e.get('subvariant')=='WSL_Base' and e.get('arch')=='$GARCH':
        print(e['link'], e.get('sha256',''))
        break")
	[ -n "$link" ] || { echo "make-guest: no Fedora WSL base in releases.json" >&2; exit 1; }
	url=${link%% *}; sha=${link##* }
	cached="$CACHE/$(basename "$url")"
	[ -f "$cached" ] || {
		echo "make-guest: fetch $(basename "$url")" >&2
		curl -sfL --retry 3 -o "$cached" "$url"
	}
	[ "$sha" = "$link" ] || echo "$sha  $cached" | sha256sum -c - >/dev/null
	BASE="$cached"
fi

# --- distro staging tree over the base ----------------------------------
# The base is extracted directly here rather than via mkimage -b so the
# VM fixups below can run before mkfs; the distro's own files come from
# a plain mkimage staging tar layered on top - same result as -b.
ST="$WORK/stage"
mkdir -p "$ST"
case "$BASE" in
*.tar.xz) tar --no-same-owner --delay-directory-restore -xJf "$BASE" -C "$ST" ;;
*.tar)    tar --no-same-owner --delay-directory-restore -xf "$BASE" -C "$ST" ;;
*)        tar --no-same-owner --delay-directory-restore -xzf "$BASE" -C "$ST" ;;
esac
find "$ST" -name '.wh.*' -delete 2>/dev/null || true
bash "$ROOT/system/linux/images/mkimage.sh" -a "$GARCH" \
	-d "$PROFILE" -o "$WORK/distro.tar"
tar --no-same-owner -xf "$WORK/distro.tar" -C "$ST"

# kernel modules for the guest: the apk ships the full set + its own
# dep files (kmod in the rootfs reads .ko.gz fine). Fedora is usrmerged,
# /lib resolves through usr/lib.
mkdir -p "$ST/usr/lib/modules"
cp -a "$WORK/kv/lib/modules/$KREL" "$ST/usr/lib/modules/"

# VM conveniences: a name, a fresh machine-id, an empty root password
# plus serial autologin, and user-mode networking on the virtio nic.
echo hibiscus > "$ST/etc/hostname"
: > "$ST/etc/machine-id"
[ -f "$ST/etc/shadow" ] && sed -i 's/^root:[^:]*:/root::/' "$ST/etc/shadow"
mkdir -p "$ST/etc/systemd/system/serial-getty@.service.d"
cat > "$ST/etc/systemd/system/serial-getty@.service.d/autologin.conf" <<'EOF'
[Service]
ExecStart=
ExecStart=-/usr/sbin/agetty --autologin root --noclear %I $TERM
EOF
# the WSL base ships NetworkManager (auto-DHCP on the virtio nic); only
# wire up systemd-networkd on bases that lack it
if [ ! -f "$ST/usr/sbin/NetworkManager" ] && \
   [ ! -f "$ST/usr/bin/NetworkManager" ] && \
   [ -f "$ST/usr/lib/systemd/systemd-networkd" ]; then
	wants="$ST/etc/systemd/system/multi-user.target.wants"
	mkdir -p "$wants" "$ST/etc/systemd/network"
	ln -sf /usr/lib/systemd/system/systemd-networkd.service \
		"$wants/systemd-networkd.service"
	ln -sf /usr/lib/systemd/system/systemd-resolved.service \
		"$wants/systemd-resolved.service"
	ln -sf /run/systemd/resolve/stub-resolv.conf "$ST/etc/resolv.conf"
	cat > "$ST/etc/systemd/network/20-wired.network" <<'EOF'
[Match]
Name=en* eth*

[Network]
DHCP=yes
EOF
fi
# the Fedora base wants a zram swap: the module is in the tree but
# nothing loads it, which costs a device timeout at boot
mkdir -p "$ST/etc/modules-load.d"
echo zram > "$ST/etc/modules-load.d/virt.conf"

# --- rootfs image ---------------------------------------------------------
sz=$(du -sm "$ST" | cut -f1)
sz=$(( sz + sz / 4 + 64 ))
truncate -s "${sz}M" "$OUT/rootfs.img"
mkfs.ext4 -q -F -d "$ST" -L hibiscus-root "$OUT/rootfs.img"

# --- manifest --------------------------------------------------------------
case "$GARCH" in
aarch64)
	MACHINE=virt; TCG=max; CON="console=tty0 console=ttyAMA0" ;;
x86_64)
	MACHINE=q35; TCG=qemu64; CON="console=tty0 console=ttyS0" ;;
esac
cat > "$OUT/guest.env" <<EOF
GUEST_ARCH=$GARCH
GUEST_QEMU=qemu-system-$GARCH
GUEST_MACHINE=$MACHINE
GUEST_CPU_KVM=host
GUEST_CPU_TCG=$TCG
GUEST_KERNEL=vmlinuz
GUEST_INITRD=initramfs.img
GUEST_ROOTFS=rootfs.img
GUEST_CMDLINE=$CON root=/dev/vda rw
EOF

echo "make-guest: wrote vmlinuz, initramfs.img, rootfs.img, guest.env to $OUT"
echo "make-guest: run 'vmd -img $OUT' or build the embedded binary with 'make dist'"

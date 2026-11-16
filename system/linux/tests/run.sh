#!/bin/bash
# Host test lane for the Linux OS: unit tests over the pure modules,
# then a live bus run - hsvcd + the sim headset + each generic driver
# on file replay, all exercised through hsvcctl exactly like the OS
# would. Ends with an mkimage assembly check.
set -e
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)
OUT=$(mktemp -d)
PIDS=""
cleanup() {
	for p in $PIDS; do kill "$p" 2>/dev/null || true; done
	rm -rf "$OUT"
}
trap cleanup EXIT

CC="${CC:-cc}"
CFLAGS="-O0 -g -Wall -Wextra -std=c99 -D_GNU_SOURCE -Iinclude -Idrivers"
mkdir -p "$OUT/bin"

echo "== build =="
make -s ARCH=$(uname -m) || make -s
BIN=out/$(uname -m)/bin
[ -x "$BIN/hsvcd" ] || BIN=out/x86_64/bin

$CC $CFLAGS -o "$OUT/bin/test_proto" tests/test_proto.c
$CC $CFLAGS -o "$OUT/bin/test_registry" tests/test_registry.c src/registry.c
$CC $CFLAGS -Ixr/hsvc -o "$OUT/bin/test_pure" tests/test_pure.c -lm
$CC $CFLAGS -Ixr/hsvc -I"$ROOT/drivers/vmd/proto" \
	-o "$OUT/bin/test_tcp" tests/test_tcp.c \
	xr/hsvc/hsvc_bus.c src/hsvc.c -lpthread

echo "== unit tests =="
"$OUT/bin/test_proto"
"$OUT/bin/test_registry"
"$OUT/bin/test_pure"
"$OUT/bin/test_tcp"

echo "== live bus =="
export HSVC_RUNDIR="$OUT/run"
mkdir -p "$HSVC_RUNDIR"
"$BIN/hsvcd" >"$OUT/hsvcd.log" 2>&1 &
PIDS="$PIDS $!"

wait_sock() {
	for _ in $(seq 50); do
		[ -S "$HSVC_RUNDIR/$1" ] && return 0
		sleep 0.1
	done
	return 1
}
wait_svc() { wait_sock "svc/$1.sock"; }
wait_sock daemon.sock || { echo "FAIL: no daemon.sock"; exit 1; }

"$BIN/hsvc-sim" >"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_svc hmd && wait_svc ctrl && wait_svc eye || { echo "FAIL: sim svcs"; exit 1; }

list=$("$BIN/hsvcctl" list)
echo "$list"
echo "$list" | grep -q "^hmd " || { echo "FAIL: hmd missing"; exit 1; }
echo "$list" | grep -q "^ctrl " || { echo "FAIL: ctrl missing"; exit 1; }
echo "$list" | grep -q "^eye " || { echo "FAIL: eye missing"; exit 1; }

path=$("$BIN/hsvcctl" resolve 1)
[ -S "$path" ] || { echo "FAIL: resolve hmd"; exit 1; }
[ -S "$("$BIN/hsvcctl" resolve 3)" ] || { echo "FAIL: resolve eye"; exit 1; }
"$BIN/hsvcctl" resolve 99 && { echo "FAIL: resolve ghost kind"; exit 1; }

info=$("$BIN/hsvcctl" info hmd)
echo "$info"
echo "$info" | grep -q "drv=sim" || { echo "FAIL: hmd info"; exit 1; }

timeout 3 "$BIN/hsvcctl" cat hmd | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: hmd stream"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat eye | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: eye stream"; exit 1; }

echo "== drivers on file replay =="
# imu: raw int sample lines
printf '100 -100 0 1000 0 -1000\n' > "$OUT/imu.txt"
"$BIN/hsvc-imu" --file "$OUT/imu.txt" >>"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_svc imu || { echo "FAIL: imu svc"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat imu | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: imu stream"; exit 1; }

# ctrl: "type code value" evdev lines - press A, push stick right
printf '1 304 1\n3 0 32767\n' > "$OUT/ctrl.txt"
"$BIN/hsvc-ctrl" --file "$OUT/ctrl.txt" --index 0 >>"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_svc ctrl0 || { echo "FAIL: ctrl0 svc"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat ctrl0 | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: ctrl stream"; exit 1; }

# eye: two bright blobs in a 64x48 gray frame, left + right halves
python3 - "$OUT/eye.raw" <<'PY'
import sys
w, h = 64, 48
img = bytearray(w * h)
for cy in range(10, 14):
    for cx in range(5, 9):      # left eye blob
        img[cy * w + cx] = 255
    for cx in range(37, 41):    # right eye blob
        img[cy * w + cx] = 255
open(sys.argv[1], "wb").write(img * 2)
PY
"$BIN/hsvc-eye" --file "$OUT/eye.raw" --width 64 --height 48 --name eyereplay >>"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_svc eyereplay || { echo "FAIL: eyereplay svc"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat eyereplay | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: replay eye stream"; exit 1; }

# two services of a kind: first registered still resolves
[ "$("$BIN/hsvcctl" resolve 3)" = "$HSVC_RUNDIR/svc/eye.sock" ] \
	|| { echo "FAIL: first eye still owns kind"; exit 1; }

echo "== mkimage =="
img="$OUT/img.tar"
bash images/mkimage.sh -a "$(uname -m)" -o "$img" -d generic >/dev/null
[ -f "$img" ] || { echo "FAIL: no image"; exit 1; }
tar -tf "$img" > "$OUT/manifest"
for want in usr/bin/hsvcd usr/bin/hsvc-sim usr/bin/hsvcctl \
	usr/lib/systemd/system/hsvcd.service \
	usr/lib/systemd/system/hsvc-svc@.service \
	usr/lib/systemd/system/hibiscus-user.service \
	usr/lib/hibiscus/mkuser usr/bin/hibiscus-mkadmin \
	etc/hibiscus/device.env etc/hibiscus/svc/sim.env \
	etc/hibiscus-release etc/systemd/system/multi-user.target.wants/hsvcd.service \
	etc/systemd/system/multi-user.target.wants/hibiscus-user.service \
	etc/systemd/system/multi-user.target.wants/hsvc-svc@sim.service; do
	grep -q "$want" "$OUT/manifest" || { echo "FAIL: image missing $want"; exit 1; }
done
# the admin helper only works setuid root - the tar must carry the bit
# and root ownership so untarring over any base keeps both
tar -tvf "$img" | grep hibiscus-mkadmin | grep -q 'rwsr-xr-x 0/0' \
	|| { echo "FAIL: mkadmin not a root-owned setuid in image"; exit 1; }
grep -q "hibiscus-linux-$(uname -m)-generic" \
	<(tar -xOf "$img" ./etc/hibiscus-release 2>/dev/null || tar -xOf "$img" etc/hibiscus-release) \
	|| { echo "FAIL: release stamp"; exit 1; }

# headset profile assembles too and wires each service up
img2="$OUT/img2.tar"
bash images/mkimage.sh -a "$(uname -m)" -o "$img2" -d headset-iio-evdev-v4l2 >/dev/null
tar -tf "$img2" | grep -q "hsvc-svc@ctrl1.service" \
	|| { echo "FAIL: headset profile ctrl1"; exit 1; }
tar -xOf "$img2" ./etc/hibiscus/svc/imu.env 2>/dev/null | grep -q "hsvc-imu" \
	|| { echo "FAIL: imu env"; exit 1; }

# mouse profile runs no local services: under a vmd host the runtime
# dials the host channel for its hmd and wand instead
img3="$OUT/img3.tar"
bash images/mkimage.sh -a "$(uname -m)" -o "$img3" -d mouse >/dev/null
tar -tf "$img3" | grep -q "etc/hibiscus/device.env" \
	|| { echo "FAIL: mouse profile device env"; exit 1; }
tar -tf "$img3" | grep -q "target.wants/hsvc-svc@" \
	&& { echo "FAIL: mouse profile still stages a service"; exit 1; }
tar -tf "$img3" | grep -q "etc/hibiscus/svc/." \
	&& { echo "FAIL: mouse profile still stages a svc env"; exit 1; }

echo "== default account =="
# mkuser against a fake root: hibiscus lands with an empty shadow
# field, a seat-less device group, and a stamp so it never runs twice
fk="$OUT/fake"
mkdir -p "$fk/etc" "$fk/bin"
printf 'root:x:0:0:root:/root:/bin/sh\ndaemon:x:1:1:d:/:/bin/sh\ntaken:x:1000:1000:t:/home/taken:/bin/sh\n' \
	> "$fk/etc/passwd"
printf 'root:x:0:\ndaemon:x:1:\nwheel:x:10:\nvideo:x:39:\naudio:x:63:pulse\n' \
	> "$fk/etc/group"
printf 'root:!:19000:0:99999:7:::\n' > "$fk/etc/shadow"
printf 'wheel:!::\n' > "$fk/etc/gshadow"
sh users/mkuser "$fk"
grep -q '^hibiscus:x:1001:1001:Hibiscus:/home/hibiscus:' "$fk/etc/passwd" \
	|| { echo "FAIL: mkuser passwd"; exit 1; }
grep -q '^hibiscus::[0-9]*:0:99999:7:::$' "$fk/etc/shadow" \
	|| { echo "FAIL: mkuser shadow not empty"; exit 1; }
grep -q '^hibiscus:x:1001:$' "$fk/etc/group" \
	|| { echo "FAIL: mkuser group"; exit 1; }
grep -q '^video:x:39:hibiscus$' "$fk/etc/group" \
	|| { echo "FAIL: mkuser video group"; exit 1; }
grep -q '^audio:x:63:pulse,hibiscus$' "$fk/etc/group" \
	|| { echo "FAIL: mkuser audio group"; exit 1; }
grep -q '^wheel:x:10:$' "$fk/etc/group" \
	|| { echo "FAIL: hibiscus must not join wheel"; exit 1; }
[ -d "$fk/home/hibiscus" ] || { echo "FAIL: mkuser home"; exit 1; }
[ -f "$fk/var/lib/hibiscus/mkuser.done" ] \
	|| { echo "FAIL: mkuser stamp"; exit 1; }
sh users/mkuser "$fk"
[ "$(grep -c '^hibiscus:' "$fk/etc/passwd")" = 1 ] \
	|| { echo "FAIL: mkuser not idempotent"; exit 1; }

# mkadmin against the same fake root: the empty admin group means the
# first account is a bootstrap any local login may run
HIBISCUS_MKADMIN_ROOT="$fk" "$BIN/hibiscus-mkadmin" alice
grep -q '^alice:x:1002:1002:Hibiscus admin:/home/alice:' "$fk/etc/passwd" \
	|| { echo "FAIL: mkadmin passwd"; exit 1; }
grep -q '^wheel:x:10:alice$' "$fk/etc/group" \
	|| { echo "FAIL: mkadmin wheel member"; exit 1; }
grep -q '^alice::[0-9]*:0:99999:7:::$' "$fk/etc/shadow" \
	|| { echo "FAIL: mkadmin shadow"; exit 1; }
[ -d "$fk/home/alice" ] || { echo "FAIL: mkadmin home"; exit 1; }
HIBISCUS_MKADMIN_ROOT="$fk" "$BIN/hibiscus-mkadmin" alice \
	&& { echo "FAIL: mkadmin took a taken name"; exit 1; }
HIBISCUS_MKADMIN_ROOT="$fk" "$BIN/hibiscus-mkadmin" 0bad \
	&& { echo "FAIL: mkadmin took a bad name"; exit 1; }
# once wheel has a member a plain login cannot mint another admin -
# the suite runs as root in CI where mkadmin skips that check, so
# this one call drops to nobody first
as_nobody=()
if [ "$(id -u)" = 0 ]; then
	chmod a+x "$OUT"
	as_nobody=(setpriv --reuid 65534 --regid 65534 --clear-groups)
fi
HIBISCUS_MKADMIN_ROOT="$fk" "${as_nobody[@]}" "$BIN/hibiscus-mkadmin" bob \
	&& { echo "FAIL: mkadmin minted a second admin"; exit 1; }
# no admin group at all: nothing grants sudo, refuse
fk2="$OUT/fake2"
mkdir -p "$fk2/etc"
printf 'root:x:0:0:root:/root:/bin/sh\n' > "$fk2/etc/passwd"
printf 'root:x:0:\n' > "$fk2/etc/group"
HIBISCUS_MKADMIN_ROOT="$fk2" "$BIN/hibiscus-mkadmin" alice \
	&& { echo "FAIL: mkadmin with no admin group"; exit 1; }

echo "== image with xr + shell =="
# the full stack path: when the host carries a matching-arch runtime
# and the dash's wayland deps, make shell + mkimage must produce an
# image that enables monado and the shell
mach="X86-64"; [ "$(uname -m)" = aarch64 ] && mach="AArch64"
if [ -f out/xr/lib/libopenxr_monado.so ] && \
	readelf -h out/xr/lib/libopenxr_monado.so | grep -q "Machine:.*$mach" && \
	command -v pkg-config >/dev/null && \
	pkg-config --exists wayland-server xkbcommon libpng libdrm openxr; then
	make -s shell
	img4="$OUT/img4.tar"
	bash images/mkimage.sh -a "$(uname -m)" -o "$img4" -d generic >/dev/null
	tar -tf "$img4" > "$OUT/manifest4"
	for want in usr/bin/dash-wayland usr/bin/monado-service \
		usr/lib/libopenxr_monado.so usr/lib/libopenxr_loader.so.1 \
		usr/lib/libopenxr.so.1 etc/openxr/1/active_runtime.json \
		etc/systemd/system/multi-user.target.wants/monado.service \
		etc/systemd/system/multi-user.target.wants/hibiscus-shell.service; do
		grep -q "$want" "$OUT/manifest4" \
			|| { echo "FAIL: xr image missing $want"; exit 1; }
	done
	tar -xOf "$img4" ./usr/lib/systemd/system/hibiscus-shell.service \
		> "$OUT/shell-unit"
	for env in "XDG_RUNTIME_DIR=/run/hibiscus" \
		"XR_RUNTIME_JSON=/etc/openxr/1/active_runtime.json"; do
		grep -q "$env" "$OUT/shell-unit" \
			|| { echo "FAIL: shell unit missing $env"; exit 1; }
	done
	grep -q "RuntimeDirectory" "$OUT/shell-unit" \
		&& { echo "FAIL: shell must not own /run/hibiscus"; exit 1; }
	tar -xOf "$img4" ./usr/lib/systemd/system/monado.service \
		> "$OUT/monado-unit"
	grep -q "RuntimeDirectory=hibiscus" "$OUT/monado-unit" \
		|| { echo "FAIL: monado unit must own /run/hibiscus"; exit 1; }
	# the image is self-sufficient: every shared lib the staged xr
	# binaries need is either vendored under usr/lib/hibiscus, aliased
	# in usr/lib, or the glibc baseline any systemd base carries
	mkdir -p "$OUT/img4x"
	tar -xf "$img4" -C "$OUT/img4x" ./usr/bin/dash-wayland \
		./usr/bin/monado-service ./usr/lib/libopenxr_monado.so
	for elf in "$OUT/img4x/usr/bin/dash-wayland" \
		"$OUT/img4x/usr/bin/monado-service" \
		"$OUT/img4x/usr/lib/libopenxr_monado.so"; do
		for n in $(readelf -d "$elf" | \
			sed -n 's/.*Shared library: \[\(.*\)\].*/\1/p'); do
			case "$n" in
			libc.so.*|libm.so.*|libdl.so.*|librt.so.*| \
			libpthread.so.*|libresolv.so.*|ld-linux*|linux-vdso*)
				continue ;;
			esac
			grep -q "usr/lib.*/$n\$" "$OUT/manifest4" || {
				echo "FAIL: $elf dep $n not vendored in image"
				exit 1; }
		done
	done
else
	echo "skip: no matching-arch runtime or dash deps on host"
fi

echo "== cross build =="
if command -v aarch64-linux-gnu-gcc >/dev/null; then
	SRARG=""
	if ! echo 'int main(void){return 0;}' | \
		aarch64-linux-gnu-gcc -xc - -o "$OUT/a64t" 2>/dev/null; then
		# bare cross gcc: needs a libc sysroot
		SR="${AARCH64_SYSROOT:-}"
		if [ -z "$SR" ] && [ -f images/fetch-sysroot.sh ]; then
			bash images/fetch-sysroot.sh "$OUT/sysroot" >/dev/null 2>&1 \
				&& SR="$OUT/sysroot/root"
		fi
		[ -n "$SR" ] && SRARG="SYSROOT=$SR"
	fi
	make -s ARCH=aarch64 $SRARG
	[ -x out/aarch64/bin/hsvcd ] || { echo "FAIL: aarch64 hsvcd"; exit 1; }
	file out/aarch64/bin/hsvcd | grep -qi aarch64 \
		|| { echo "FAIL: aarch64 binary"; exit 1; }
	SRDIR="${SRARG#SYSROOT=}"
else
	echo "skip: no aarch64-linux-gnu-gcc"
fi

echo "== cross image with xr + shell =="
# the full aarch64 path: when out/xr-aarch64 carries a real AArch64
# runtime and the cross-built shell, mkimage -a aarch64 must assemble
# an image with monado + the shell enabled
if [ -f out/xr-aarch64/lib/libopenxr_monado.so ] && \
	readelf -h out/xr-aarch64/lib/libopenxr_monado.so | grep -q "Machine:.*AArch64" && \
	readelf -h out/xr-aarch64/bin/dash-wayland 2>/dev/null | grep -q "Machine:.*AArch64" && \
	command -v aarch64-linux-gnu-gcc >/dev/null; then
	img5="$OUT/img5.tar"
	SYSROOT="${SRDIR:-${AARCH64_SYSROOT:-}}" \
		bash images/mkimage.sh -a aarch64 -o "$img5" -d generic >/dev/null
	tar -tf "$img5" > "$OUT/manifest5"
	for want in usr/bin/dash-wayland usr/bin/monado-service \
		usr/lib/libopenxr_monado.so usr/lib/libopenxr_loader.so.1 \
		usr/lib/libopenxr.so.1 etc/openxr/1/active_runtime.json \
		etc/systemd/system/multi-user.target.wants/monado.service \
		etc/systemd/system/multi-user.target.wants/hibiscus-shell.service; do
		grep -q "$want" "$OUT/manifest5" \
			|| { echo "FAIL: aarch64 image missing $want"; exit 1; }
	done
	mkdir -p "$OUT/img5x"
	tar -xf "$img5" -C "$OUT/img5x" ./usr/bin/dash-wayland \
		./usr/bin/monado-service ./usr/lib/libopenxr_monado.so
	for elf in "$OUT/img5x/usr/bin/dash-wayland" \
		"$OUT/img5x/usr/bin/monado-service" \
		"$OUT/img5x/usr/lib/libopenxr_monado.so"; do
		for n in $(readelf -d "$elf" | \
			sed -n 's/.*Shared library: \[\(.*\)\].*/\1/p'); do
			case "$n" in
			libc.so.*|libm.so.*|libdl.so.*|librt.so.*| \
			libpthread.so.*|libresolv.so.*|ld-linux*|linux-vdso*)
				continue ;;
			esac
			grep -q "usr/lib.*/$n\$" "$OUT/manifest5" || {
				echo "FAIL: $elf dep $n not vendored in image"
				exit 1; }
		done
	done
else
	echo "skip: no aarch64 runtime+shell in out/xr-aarch64 or no cross gcc"
fi

echo "ALL TESTS PASS"

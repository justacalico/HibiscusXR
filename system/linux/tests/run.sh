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

echo "== unit tests =="
"$OUT/bin/test_proto"
"$OUT/bin/test_registry"
"$OUT/bin/test_pure"

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

# vmd: "type code value" evdev lines - move the mouse, left click
printf '2 0 40\n2 1 -10\n1 272 1\n' > "$OUT/vmd.txt"
"$BIN/hsvc-vmd" --file "$OUT/vmd.txt" >>"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_svc vmd-hmd || { echo "FAIL: vmd-hmd svc"; exit 1; }
wait_svc vmd-ctrl || { echo "FAIL: vmd-ctrl svc"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat vmd-hmd | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: vmd hmd stream"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat vmd-ctrl | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: vmd ctrl stream"; exit 1; }

echo "== standalone vmd =="
# a fresh rundir with no daemon: the driver embeds the broker and
# brings its own bus up - that is what makes the single binary the
# whole stack
export HSVC_RUNDIR="$OUT/alone"
"$BIN/hsvc-vmd" --file "$OUT/vmd.txt" >>"$OUT/sim.log" 2>&1 &
PIDS="$PIDS $!"
wait_sock daemon.sock || { echo "FAIL: embedded broker"; exit 1; }
wait_svc vmd-hmd || { echo "FAIL: standalone vmd-hmd"; exit 1; }
timeout 3 "$BIN/hsvcctl" cat vmd-hmd | head -1 | grep -q "stream=0" \
	|| { echo "FAIL: standalone stream"; exit 1; }
export HSVC_RUNDIR="$OUT/run"

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

# mouse profile wires the virtual motion device up
img3="$OUT/img3.tar"
bash images/mkimage.sh -a "$(uname -m)" -o "$img3" -d mouse >/dev/null
tar -tf "$img3" | grep -q "hsvc-svc@vmd.service" \
	|| { echo "FAIL: mouse profile vmd"; exit 1; }
tar -xOf "$img3" ./etc/hibiscus/svc/vmd.env 2>/dev/null | grep -q "hsvc-vmd" \
	|| { echo "FAIL: vmd env"; exit 1; }

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
# once wheel has a member a plain login cannot mint another admin
HIBISCUS_MKADMIN_ROOT="$fk" "$BIN/hibiscus-mkadmin" bob \
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
		usr/lib/libopenxr_monado.so etc/openxr/1/active_runtime.json \
		etc/systemd/system/multi-user.target.wants/monado.service \
		etc/systemd/system/multi-user.target.wants/hibiscus-shell.service; do
		grep -q "$want" "$OUT/manifest4" \
			|| { echo "FAIL: xr image missing $want"; exit 1; }
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
		usr/lib/libopenxr_monado.so etc/openxr/1/active_runtime.json \
		etc/systemd/system/multi-user.target.wants/monado.service \
		etc/systemd/system/multi-user.target.wants/hibiscus-shell.service; do
		grep -q "$want" "$OUT/manifest5" \
			|| { echo "FAIL: aarch64 image missing $want"; exit 1; }
	done
else
	echo "skip: no aarch64 runtime+shell in out/xr-aarch64 or no cross gcc"
fi

echo "== dist =="
# the single-file build: one static binary per arch, no deps at all.
# Needs a static libc per toolchain - libc6-dev on debian, glibc-static
# on fedora, the fetched sysroot for a bare aarch64 cross gcc
if echo 'int main(void){return 0;}' | \
	gcc -static -xc - -o "$OUT/st" 2>/dev/null; then
	make -s dist-host
	file "out/dist/hsvc-vmd-$(uname -m)" | grep -qi "statically linked" \
		|| { echo "FAIL: dist binary not static"; exit 1; }
	export HSVC_RUNDIR="$OUT/dist-run"
	"out/dist/hsvc-vmd-$(uname -m)" --file "$OUT/vmd.txt" >>"$OUT/sim.log" 2>&1 &
	PIDS="$PIDS $!"
	wait_svc vmd-hmd || { echo "FAIL: dist standalone"; exit 1; }
	export HSVC_RUNDIR="$OUT/run"
else
	echo "skip: no host static libc"
fi
if command -v aarch64-linux-gnu-gcc >/dev/null && \
	echo 'int main(void){return 0;}' | \
	aarch64-linux-gnu-gcc ${SRDIR:+--sysroot=$SRDIR} -static -xc - \
		-o "$OUT/a64st" 2>/dev/null; then
	make -s out/dist/hsvc-vmd-aarch64 A64SYSROOT="$SRDIR"
	file out/dist/hsvc-vmd-aarch64 | grep -qi aarch64 \
		|| { echo "FAIL: aarch64 dist"; exit 1; }
	file out/dist/hsvc-vmd-aarch64 | grep -qi "statically linked" \
		|| { echo "FAIL: aarch64 dist not static"; exit 1; }
	readelf -lW out/dist/hsvc-vmd-aarch64 | grep -q "LOAD.*0x4000" \
		|| { echo "FAIL: aarch64 dist not 16k-page safe"; exit 1; }
else
	echo "skip: no aarch64 static libc"
fi

echo "ALL TESTS PASS"

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
	etc/hibiscus/device.env etc/hibiscus/svc/sim.env \
	etc/hibiscus-release etc/systemd/system/multi-user.target.wants/hsvcd.service \
	etc/systemd/system/multi-user.target.wants/hsvc-svc@sim.service; do
	grep -q "$want" "$OUT/manifest" || { echo "FAIL: image missing $want"; exit 1; }
done
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

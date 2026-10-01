#!/bin/bash
# Host-side cted smoke test: builds the daemon for the local machine and
# exercises the wire protocol the same way applications/desktop/cte does.
set -e
cd "$(dirname "$0")/.."
ROOT=$(git rev-parse --show-toplevel)
OUT=$(mktemp -d)
trap "kill %1 2>/dev/null; rm -rf $OUT" EXIT

PORT=17340
CC="${CC:-cc}"
"$CC" -O0 -g -o "$OUT/cted" cted.c \
  -I"$ROOT/applications/vrhome/src/input" \
  "$ROOT/applications/vrhome/src/input/ctrl_state.c" -lpthread

# fake sharemem: reuse the app test's layout - both blocks DONE, data set
python3 - "$OUT/sharemem" <<'PY'
import struct, sys
buf = bytearray(1024)
for w in (0, 1):
    buf[w * 512] = 2
    buf[w * 512 + 100] = 2
    base = w * 512 + 4
    struct.pack_into(">fffffff", buf, base + 40, 0.1, 0.2, 0.3, 0.9, 0.1, 0.2, 0.3)
    struct.pack_into(">i", buf, base + 68, 1)
    kb = w * 512 + 104
    struct.pack_into(">iiiiiii", buf, kb, 128, 128, 0, 0, 0, 0, 80 + w)
open(sys.argv[1], "wb").write(buf)
PY

CTE_PORT=$PORT CTE_CTRL_PATH="$OUT/sharemem" "$OUT/cted" &
sleep 0.4

req() {
  python3 - "$PORT" "$1" <<'PY'
import socket, sys
s = socket.create_connection(("127.0.0.1", int(sys.argv[1])), timeout=5)
s.recv(64)  # banner
s.sendall((sys.argv[2] + "\n").encode())
out = b""
s.settimeout(1.0)
try:
    while True:
        c = s.recv(65536)
        if not c: break
        out += c
except socket.timeout:
    pass
print(out.decode("utf-8", "replace"), end="")
PY
}

echo "--- banner + PING ---"
req "PING" | head -2
echo "--- INFO ---"
req "INFO" | head -3
echo "--- CTRL ---"
req "CTRL" | head -3
echo "--- unknown ---"
req "WAT" | head -2
echo "PASS"

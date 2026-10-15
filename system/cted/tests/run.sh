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

# deterministic tiny tar.gz regardless of host tar quirks
python3 - "$OUT/blob.tgz" <<'PY'
import gzip, sys
def hdr(name, size):
    h = bytearray(512)
    h[0:len(name)] = name.encode()
    h[100:108] = b"0000644\0"
    h[124:136] = ("%011o\0" % size).encode()
    h[136:148] = b"00000000000\0"
    h[156:157] = b"0"
    h[257:265] = b"ustar\0000"
    c = sum(h[:156]) + sum(b"        ") + sum(h[165:])
    h[148:156] = ("%06o\0 " % c).encode()
    return h
body = b"fake dump body\n"
t = bytes(hdr("meta.txt", len(body))) + body
t += bytes(512 - len(body) % 512)
t += bytes(1024)
open(sys.argv[1], "wb").write(gzip.compress(t))
PY
cat >"$OUT/fake-dump.sh" <<SH
#!/bin/sh
cat "$OUT/blob.tgz"
SH
chmod +x "$OUT/fake-dump.sh"

CTE_PORT=$PORT CTE_CTRL_PATH="$OUT/sharemem" CTE_DUMP="$OUT/fake-dump.sh" \
  CTE_TMPDIR="$OUT" "$OUT/cted" &
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
echo "--- DUMP ---"
req "DUMP" | head -1 | grep -q '^+BLOB ' && echo "dump blob ok"
echo "--- unknown ---"
req "WAT" | head -2
echo "PASS"

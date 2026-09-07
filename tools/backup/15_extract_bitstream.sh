#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
E=${PN2_ROOT}/eyeunit
L=${PN2_ROOT}/notes/15_bitstream.log
exec >"$L" 2>&1

echo "=== reproducibility ==="
md5sum "$E/eye_262144.bin" "$E/eye_verify.bin"
if cmp -s "$E/eye_262144.bin" "$E/eye_verify.bin"; then
  echo "IDENTICAL across two independent reads - dump is stable"
else
  echo "DIFFER - read is NOT stable"
  cmp -l "$E/eye_262144.bin" "$E/eye_verify.bin" | head -20
fi
echo

echo "=== tail of the data region ==="
python3 - "$E/eye_262144.bin" <<'PY'
import sys
d=open(sys.argv[1],'rb').read()
last=max(i for i,b in enumerate(d) if b!=0xFF)
print("last non-0xFF byte: 0x%06x (%d)  -> payload length %d" % (last,last,last+1))
s=last-95
for j in range(s, last+17, 16):
    row=d[j:j+16]
    print("%06x  %-48s |%s|" % (j, ' '.join('%02x'%b for b in row),
          ''.join(chr(b) if 32<=b<127 else '.' for b in row)))
PY
echo

echo "=== carve candidates ==="
python3 - "$E" <<'PY'
import sys, os
E=sys.argv[1]
d=open(os.path.join(E,'eye_262144.bin'),'rb').read()
last=max(i for i,b in enumerate(d) if b!=0xFF)

# candidate A: exact payload, first byte through last non-FF
a=d[:last+1]
open(os.path.join(E,'top_level_bitmap_A_exact.bin'),'wb').write(a)
print("A exact      : %d bytes (0x%x)" % (len(a),len(a)))

# candidate B: pad up to next 256-byte page boundary with 0xFF
plen=((last+1)+255)//256*256
b=d[:plen]
open(os.path.join(E,'top_level_bitmap_B_page.bin'),'wb').write(b)
print("B page-pad   : %d bytes (0x%x)" % (len(b),len(b)))

# candidate C: full 32KB region (0x8000) as the driver's natural block
c=d[:0x8000]
open(os.path.join(E,'top_level_bitmap_C_32k.bin'),'wb').write(c)
print("C 32KB       : %d bytes (0x%x)" % (len(c),len(c)))

# candidate D: whole 256KB as read
open(os.path.join(E,'top_level_bitmap_D_full256k.bin'),'wb').write(d)
print("D full 256KB : %d bytes (0x%x)" % (len(d),len(d)))
PY
echo

echo "=== iCE40 bitstream sanity ==="
python3 - "$E/top_level_bitmap_A_exact.bin" <<'PY'
import sys
d=open(sys.argv[1],'rb').read()
# ASCII comment header is NUL-delimited records before the 7EAA997E preamble
pre=d.find(bytes.fromhex('7eaa997e'))
print("preamble 7EAA997E at 0x%x" % pre)
hdr=d[:pre]
recs=[r.decode('ascii','replace') for r in hdr.split(b'\x00') if r.strip(b'\xff')]
print("header records:")
for r in recs: print("   %r" % r)
print()
print("config payload length after preamble: %d bytes" % (len(d)-pre))
print("expected iCE40LP1K config ~32220 bytes")
PY
echo

echo "=== final listing ==="
ls -l "$E"
md5sum "$E"/top_level_bitmap_*.bin
echo DONE

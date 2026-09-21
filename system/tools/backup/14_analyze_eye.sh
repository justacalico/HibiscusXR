#!/bin/bash
PN2_ROOT="${PN2_ROOT:-$HOME/PN2Lineage}"
set -u
F=${PN2_ROOT}/eyeunit/eye_262144.bin
L=${PN2_ROOT}/notes/14_eye_dump.log
exec >"$L" 2>&1

echo "=== file ==="
ls -l "$F"
file -b "$F"
md5sum "$F"
sha256sum "$F"
echo

echo "=== head 512 ==="
xxd -l 512 "$F"
echo

echo "=== occupancy map: which 4KB blocks contain non-0xFF data? ==="
python3 - "$F" <<'PY'
import sys
d=open(sys.argv[1],'rb').read()
BS=4096
runs=[]
for i in range(0,len(d),BS):
    blk=d[i:i+BS]
    nonff=sum(1 for b in blk if b!=0xFF)
    runs.append((i,nonff,len(blk)))
# summarise contiguous used/unused
prev=None; start=0
for off,nonff,ln in runs:
    used = nonff>0
    if prev is None:
        prev=used; start=off
    elif used!=prev:
        print("0x%06x - 0x%06x  %s" % (start, off-1, "DATA" if prev else "erased(0xFF)"))
        prev=used; start=off
print("0x%06x - 0x%06x  %s" % (start, len(d)-1, "DATA" if prev else "erased(0xFF)"))
print()
tot=sum(r[1] for r in runs)
print("total non-0xFF bytes: %d / %d (%.1f%%)" % (tot, len(d), 100.0*tot/len(d)))
# last offset with any non-FF
last=max((i for i,b in enumerate(d) if b!=0xFF), default=-1)
print("last non-0xFF byte at offset: 0x%06x (%d)" % (last,last))
PY
echo

echo "=== byte histogram (top 12) ==="
od -An -v -t x1 "$F" | tr ' ' '\n' | grep -v '^$' | sort | uniq -c | sort -rn | head -12
echo

echo "=== printable strings (min 4) ==="
strings -n 4 "$F" | head -60
echo

echo "=== FPGA / toolchain markers ==="
strings -n 3 "$F" | grep -iE 'lattice|xilinx|altera|quartus|vivado|ice|machxo|ecp|spartan|artix|cyclone|max10|bitstream|\.jed|\.bit' | head -20
echo "(sync-word scan)"
python3 - "$F" <<'PY'
import sys
d=open(sys.argv[1],'rb').read()
pats={
 'Xilinx sync aa995566': bytes.fromhex('aa995566'),
 'Xilinx bus-width bus': bytes.fromhex('000000bb'),
 'Lattice 0xFFFFBDB3' : bytes.fromhex('ffffbdb3'),
 'Lattice preamble 7EAA997E': bytes.fromhex('7eaa997e'),
 'Altera/Intel 6A6A'  : bytes.fromhex('6a6a'),
}
for n,p in pats.items():
    i=d.find(p)
    print("%-24s %s" % (n, ("found at 0x%06x"%i) if i>=0 else "not found"))
PY
echo

echo "=== first 64 bytes of first DATA region (skip leading 0xFF) ==="
python3 - "$F" <<'PY'
import sys
d=open(sys.argv[1],'rb').read()
i=0
while i<len(d) and d[i]==0xFF: i+=1
print("first non-0xFF at 0x%06x" % i)
s=max(0,i-16)
chunk=d[s:s+160]
for j in range(0,len(chunk),16):
    row=chunk[j:j+16]
    print("%06x  %-48s |%s|" % (s+j, ' '.join('%02x'%b for b in row),
          ''.join(chr(b) if 32<=b<127 else '.' for b in row)))
PY
echo DONE

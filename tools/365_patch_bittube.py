#!/usr/bin/env python3
"""Make sp<BitTube>::operator=(sp&&) tolerate a null source.

CVService's 32-bit getPvrService() issues transact 0x3b, builds a BitTube from the
reply without checking the transact result, and hands the destination a null
source. operator= then does `ldr r0,[r5]` with r5 == 0 and takes SIGSEGV. Each
crash respawns the service, which hammers pvrservice with
registerClient/binderDied until it deadlocks in fork-vs-jemalloc - and that wedge
is what leaves VRShell and the see-through app stuck on their 2D loading screens.

No code cave needed. The function already carries 8 bytes of dead weight:

    109b0: 4286        cmp   r6, r0
    109b2: bf18        it    ne
    109b4: f7fa ea12   blxne android::sp_report_race()

That is a debug-only refcount race check. Replace it with the null guard:

    109b0: 2d00        cmp   r5, #0
    109b2: d005        beq   0x109c0        -> mov r0,r4 / pop {r4,r5,r6,pc}
    109b4: bf00        nop
    109b6: bf00        nop

On the null path the destination keeps its existing value and r0 is already the
destination, so the return value stays correct. Everything from 0x109b8 on is
untouched.
"""
import struct, sys, shutil

SRC, DST = sys.argv[1], sys.argv[2]

VADDR = 0x109B0
ORIG = bytes([0x86, 0x42, 0x18, 0xBF, 0xFA, 0xF7, 0x12, 0xEA])
NEW  = bytes([0x00, 0x2D, 0x05, 0xD0, 0x00, 0xBF, 0x00, 0xBF])


def vaddr_to_off(data, vaddr):
    """Walk PT_LOAD headers; .text vaddr is not necessarily its file offset."""
    assert data[:4] == b"\x7fELF" and data[4] == 1, "expected 32-bit ELF"
    e_phoff = struct.unpack_from("<I", data, 0x1C)[0]
    e_phentsize = struct.unpack_from("<H", data, 0x2A)[0]
    e_phnum = struct.unpack_from("<H", data, 0x2C)[0]
    for i in range(e_phnum):
        p = e_phoff + i * e_phentsize
        p_type, p_offset, p_vaddr, _, p_filesz = struct.unpack_from("<IIIII", data, p)
        if p_type == 1 and p_vaddr <= vaddr < p_vaddr + p_filesz:
            return p_offset + (vaddr - p_vaddr)
    raise SystemExit(f"vaddr {vaddr:#x} not in any PT_LOAD")


shutil.copyfile(SRC, DST)
data = bytearray(open(DST, "rb").read())

off = vaddr_to_off(data, VADDR)
print(f"vaddr {VADDR:#x} -> file offset {off:#x}")

cur = bytes(data[off:off + 8])
print(f"  found    {cur.hex(' ')}")
print(f"  expected {ORIG.hex(' ')}")
if cur == NEW:
    raise SystemExit("already patched, nothing to do")
if cur != ORIG:
    raise SystemExit("FAIL: bytes do not match the sp_report_race check, refusing")

data[off:off + 8] = NEW
open(DST, "wb").write(data)
print(f"  wrote    {NEW.hex(' ')}")
print(f"patched {DST}")

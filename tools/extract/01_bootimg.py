#!/usr/bin/env python3
"""Parse Android boot.img (hdr v0-v3), split out kernel/ramdisk/dtb, report os_version."""
import struct, sys, os, zlib

def u32(b, o): return struct.unpack_from('<I', b, o)[0]
def u64(b, o): return struct.unpack_from('<Q', b, o)[0]

def parse(path, outdir):
    os.makedirs(outdir, exist_ok=True)
    d = open(path, 'rb').read()
    assert d[:8] == b'ANDROID!', 'not an android boot image: %r' % d[:8]

    ksz, kaddr = u32(d, 8), u32(d, 12)
    rsz, raddr = u32(d, 16), u32(d, 20)
    ssz, saddr = u32(d, 24), u32(d, 28)
    tags = u32(d, 32)
    page = u32(d, 36)
    hver = u32(d, 40)
    osver = u32(d, 44)
    name = d[48:64].rstrip(b'\0').decode('utf8', 'replace')
    cmdline = d[64:64+512].rstrip(b'\0').decode('utf8', 'replace')
    extra = d[608:608+1024].rstrip(b'\0').decode('utf8', 'replace')

    # os_version packs version + security patch date
    v = osver >> 11
    maj, minr, pat = (v >> 14) & 0x7f, (v >> 7) & 0x7f, v & 0x7f
    dt = osver & 0x7ff
    yr, mo = ((dt >> 4) & 0x7f) + 2000, dt & 0xf

    print('=== boot.img header ===')
    print('path            %s (%d bytes)' % (path, len(d)))
    print('header_version  %d' % hver)
    print('page_size       %d' % page)
    print('product name    %r' % name)
    print('OS version      %d.%d.%d' % (maj, minr, pat))
    print('security patch  %04d-%02d' % (yr, mo))
    print('kernel          size=%d addr=0x%08x' % (ksz, kaddr))
    print('ramdisk         size=%d addr=0x%08x' % (rsz, raddr))
    print('second          size=%d addr=0x%08x' % (ssz, saddr))
    print('tags_addr       0x%08x' % tags)
    print()
    print('--- cmdline ---')
    for tok in cmdline.split():
        print('   ' + tok)
    if extra:
        print('--- extra_cmdline ---')
        for tok in extra.split():
            print('   ' + tok)
    print()

    def pages(n): return (n + page - 1) // page * page
    off = page
    parts = [('kernel', ksz), ('ramdisk', rsz), ('second', ssz)]

    dtbsz = 0
    if hver >= 1:
        rdtbo_sz = u32(d, 1632)
        hdr_sz = u32(d, 1648)
        print('recovery_dtbo   size=%d' % rdtbo_sz)
        print('header_size     %d' % hdr_sz)
        parts.append(('recovery_dtbo', rdtbo_sz))
    if hver >= 2:
        dtbsz = u32(d, 1652)
        dtbaddr = u64(d, 1656)
        print('dtb             size=%d addr=0x%016x' % (dtbsz, dtbaddr))
        parts.append(('dtb', dtbsz))
    print()

    print('=== extracted ===')
    for nm, sz in parts:
        if sz == 0:
            continue
        blob = d[off:off+sz]
        p = os.path.join(outdir, nm + '.bin')
        open(p, 'wb').write(blob)
        print('%-14s %10d bytes  %s  magic=%s' % (nm, sz, p, blob[:4].hex()))
        off += pages(sz)

    # kernel compression + version string
    kp = os.path.join(outdir, 'kernel.bin')
    if os.path.exists(kp):
        k = open(kp, 'rb').read()
        raw = None
        if k[:2] == b'\x1f\x8b':
            print('\nkernel is gzip; decompressing')
            raw = zlib.decompress(k, 16 + zlib.MAX_WBITS)
        else:
            # gzip payload may be embedded after a stub
            i = k.find(b'\x1f\x8b\x08')
            if i > 0:
                print('\nfound embedded gzip at 0x%x; decompressing' % i)
                try:
                    raw = zlib.decompressobj(16 + zlib.MAX_WBITS).decompress(k[i:])
                except Exception as e:
                    print('  gzip failed: %s' % e)
            else:
                print('\nkernel magic %s (not gzip)' % k[:8].hex())
        if raw:
            open(os.path.join(outdir, 'kernel.raw'), 'wb').write(raw)
            print('  decompressed to kernel.raw (%d bytes)' % len(raw))
            j = raw.find(b'Linux version ')
            if j >= 0:
                print('  ' + raw[j:j+220].split(b'\0')[0].decode('utf8', 'replace'))
        else:
            j = k.find(b'Linux version ')
            if j >= 0:
                print('  ' + k[j:j+220].split(b'\0')[0].decode('utf8', 'replace'))

if __name__ == '__main__':
    parse(sys.argv[1], sys.argv[2])

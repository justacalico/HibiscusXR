#!/usr/bin/env python3
"""Split an Android DTBO image into its constituent FDT blobs, and scan a kernel
image for an appended DTB. Both are needed to reconstruct a device tree."""
import struct, sys, os, zlib

FDT_MAGIC = bytes.fromhex('d00dfeed')
DTBO_MAGIC = 0xd7b7ab1e

def split_dtbo(path, outdir):
    d = open(path, 'rb').read()
    magic, total, hdrsz, entsz, entcnt, entoff, pagesz, ver = struct.unpack('>8I', d[:32])
    print('=== DTBO header ===')
    print('magic          0x%08x %s' % (magic, 'OK' if magic == DTBO_MAGIC else 'MISMATCH'))
    if magic != DTBO_MAGIC:
        print('not a dtbo image; scanning raw for FDT blobs instead')
        return scan_fdt(d, outdir, 'raw')
    print('total_size     %d' % total)
    print('entry_size     %d' % entsz)
    print('entry_count    %d' % entcnt)
    print('entries_offset %d' % entoff)
    print('page_size      %d' % pagesz)
    print('version        %d' % ver)
    print()
    os.makedirs(outdir, exist_ok=True)
    out = []
    for i in range(entcnt):
        off = entoff + i * entsz
        dt_size, dt_off, did, rev = struct.unpack('>4I', d[off:off+16])
        blob = d[dt_off:dt_off+dt_size]
        p = os.path.join(outdir, 'dtbo_%02d.dtb' % i)
        open(p, 'wb').write(blob)
        ok = blob[:4] == FDT_MAGIC
        print('entry %2d  size=%-8d offset=0x%-8x id=0x%-8x rev=0x%-8x  fdt=%s  -> %s'
              % (i, dt_size, dt_off, did, rev, 'OK' if ok else 'BAD', os.path.basename(p)))
        out.append(p)
    return out

def scan_fdt(data, outdir, tag):
    """Find embedded FDT blobs by magic + plausible totalsize."""
    os.makedirs(outdir, exist_ok=True)
    found, i = [], 0
    while True:
        i = data.find(FDT_MAGIC, i)
        if i < 0:
            break
        if i + 8 <= len(data):
            total = struct.unpack('>I', data[i+4:i+8])[0]
            if 0x100 < total < 4 * 1024 * 1024 and i + total <= len(data):
                p = os.path.join(outdir, '%s_fdt_%06x.dtb' % (tag, i))
                open(p, 'wb').write(data[i:i+total])
                print('  FDT at 0x%08x  size=%d  -> %s' % (i, total, os.path.basename(p)))
                found.append(p)
                i += total
                continue
        i += 4
    if not found:
        print('  no embedded FDT blobs found')
    return found

if __name__ == '__main__':
    what, src, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
    if what == 'dtbo':
        split_dtbo(src, outdir)
    else:
        d = open(src, 'rb').read()
        print('=== scanning %s (%d bytes) for appended FDT ===' % (src, len(d)))
        scan_fdt(d, outdir, os.path.basename(src).split('.')[0])

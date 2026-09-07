#!/usr/bin/env python3
"""
Patch the stock boot.img ramdisk to force adb on.

ro.debuggable / ro.adb.secure are read from the ramdisk's default.prop, which
loads before /system/build.prop and wins. Patching the system image (as I first
tried) has no effect on whether adbd starts.

Rebuilds a header v0 boot image with the original addresses preserved.
"""
import struct, sys, os, gzip, io, subprocess, shutil

def u32(b, o): return struct.unpack_from('<I', b, o)[0]

PROPS = {
    'ro.debuggable': '1',
    'ro.adb.secure': '0',
    'ro.secure': '0',
    'persist.sys.usb.config': 'adb,mtp',
    'ro.allow.mock.location': '1',
}

def unpack(path, work):
    d = open(path, 'rb').read()
    assert d[:8] == b'ANDROID!', 'not a boot image'
    ksz, kaddr = u32(d, 8), u32(d, 12)
    rsz, raddr = u32(d, 16), u32(d, 20)
    ssz, saddr = u32(d, 24), u32(d, 28)
    tags = u32(d, 32); page = u32(d, 36); hver = u32(d, 40); osver = u32(d, 44)
    name = d[48:64]; cmdline = d[64:64+512]; extra = d[608:608+1024]

    print('header v%d page=%d kernel=%d ramdisk=%d second=%d' % (hver, page, ksz, rsz, ssz))
    def pages(n): return (n + page - 1)//page*page
    off = page
    kernel = d[off:off+ksz];  off += pages(ksz)
    ramdisk = d[off:off+rsz]; off += pages(rsz)
    second = d[off:off+ssz] if ssz else b''

    os.makedirs(work, exist_ok=True)
    open(os.path.join(work,'kernel'),'wb').write(kernel)
    open(os.path.join(work,'ramdisk.orig'),'wb').write(ramdisk)
    return dict(ksz=ksz,kaddr=kaddr,rsz=rsz,raddr=raddr,ssz=ssz,saddr=saddr,
                tags=tags,page=page,hver=hver,osver=osver,name=name,
                cmdline=cmdline,extra=extra,kernel=kernel,ramdisk=ramdisk,second=second)

def patch_ramdisk(work):
    rd = open(os.path.join(work,'ramdisk.orig'),'rb').read()
    print('ramdisk magic: %s' % rd[:4].hex())
    if rd[:2] == b'\x1f\x8b':
        raw = gzip.decompress(rd)
        comp = 'gzip'
    else:
        raise SystemExit('unexpected ramdisk compression %s' % rd[:4].hex())
    print('decompressed ramdisk: %d bytes (%s cpio)' % (len(raw), comp))

    ex = os.path.join(work,'rd')
    if os.path.exists(ex): shutil.rmtree(ex)
    os.makedirs(ex)
    p = subprocess.run(['cpio','-idm','--quiet'], input=raw, cwd=ex,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        print('cpio extract stderr:', p.stderr.decode()[:400])
    print('extracted entries: %d' % sum(len(f) for _,_,f in os.walk(ex)))

    # patch every prop file present in the ramdisk
    for cand in ('default.prop', 'prop.default'):
        fp = os.path.join(ex, cand)
        if not os.path.exists(fp):
            continue
        lines = open(fp, encoding='utf8', errors='replace').read().splitlines()
        out, seen = [], set()
        for ln in lines:
            k = ln.split('=')[0].strip()
            if k in PROPS:
                out.append('%s=%s' % (k, PROPS[k])); seen.add(k)
            else:
                out.append(ln)
        for k, v in PROPS.items():
            if k not in seen:
                out.append('%s=%s' % (k, v))
        open(fp, 'w', encoding='utf8').write('\n'.join(out) + '\n')
        print('patched /%s' % cand)
        for k in PROPS:
            for ln in out:
                if ln.startswith(k + '='):
                    print('   %s' % ln)

    # repack cpio (newc), preserving order
    names = []
    for root, dirs, files in os.walk(ex):
        for n in dirs + files:
            fp = os.path.join(root, n)
            names.append(os.path.relpath(fp, ex))
    names.sort()
    p = subprocess.run(['cpio','-o','-H','newc','--quiet'],
                       input='\n'.join(names).encode(), cwd=ex,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode != 0:
        print('cpio pack stderr:', p.stderr.decode()[:400])
        raise SystemExit('cpio repack failed')
    newraw = p.stdout
    print('repacked cpio: %d bytes' % len(newraw))

    buf = io.BytesIO()
    with gzip.GzipFile(fileobj=buf, mode='wb', compresslevel=9, mtime=0) as g:
        g.write(newraw)
    newrd = buf.getvalue()
    print('recompressed ramdisk: %d bytes (was %d)' % (len(newrd), len(rd)))
    open(os.path.join(work,'ramdisk.new'),'wb').write(newrd)
    return newrd

def repack(h, newrd, out):
    page = h['page']
    def pad(b):
        r = len(b) % page
        return b + (b'\0' * (page - r) if r else b'')

    hdr = bytearray(page)
    hdr[0:8] = b'ANDROID!'
    struct.pack_into('<I', hdr, 8,  len(h['kernel']))
    struct.pack_into('<I', hdr, 12, h['kaddr'])
    struct.pack_into('<I', hdr, 16, len(newrd))
    struct.pack_into('<I', hdr, 20, h['raddr'])
    struct.pack_into('<I', hdr, 24, len(h['second']))
    struct.pack_into('<I', hdr, 28, h['saddr'])
    struct.pack_into('<I', hdr, 32, h['tags'])
    struct.pack_into('<I', hdr, 36, page)
    struct.pack_into('<I', hdr, 40, h['hver'])
    struct.pack_into('<I', hdr, 44, h['osver'])
    hdr[48:64] = h['name']
    hdr[64:64+512] = h['cmdline']
    hdr[608:608+1024] = h['extra']

    blob = bytes(hdr) + pad(h['kernel']) + pad(newrd)
    if h['second']:
        blob += pad(h['second'])
    open(out,'wb').write(blob)
    print('wrote %s (%d bytes)' % (out, len(blob)))

if __name__ == '__main__':
    src, work, out = sys.argv[1], sys.argv[2], sys.argv[3]
    h = unpack(src, work)
    newrd = patch_ramdisk(work)
    repack(h, newrd, out)

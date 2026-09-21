#!/usr/bin/env python3
"""Rebuild a raw ext4 image from Android OTA <part>.new.dat + <part>.transfer.list."""
import sys, os

def main(tlist, datfile, out):
    lines = open(tlist).read().splitlines()
    ver = int(lines[0])
    total_blocks = int(lines[1])
    cmds = lines[4:] if ver >= 2 else lines[2:]

    # collect 'new' commands only; zero/erase ranges are just holes
    ranges = []
    for ln in cmds:
        if not ln.strip():
            continue
        parts = ln.split(' ', 1)
        if parts[0] != 'new':
            continue
        toks = [int(x) for x in parts[1].split(',')]
        n = toks[0]
        vals = toks[1:]
        for i in range(0, n, 2):
            ranges.append((vals[i], vals[i + 1]))

    maxblk = max(e for _, e in ranges) if ranges else total_blocks
    print('transfer list v%d, %d total blocks, %d new ranges, max block %d'
          % (ver, total_blocks, len(ranges), maxblk))

    BS = 4096
    src = open(datfile, 'rb')
    with open(out, 'wb') as o:
        o.truncate(maxblk * BS)
        for begin, end in ranges:
            o.seek(begin * BS)
            remaining = (end - begin) * BS
            while remaining > 0:
                chunk = src.read(min(remaining, 1 << 22))
                if not chunk:
                    print('WARNING: dat exhausted early at block %d' % begin)
                    break
                o.write(chunk)
                remaining -= len(chunk)
    src.close()
    print('wrote %s (%d bytes)' % (out, os.path.getsize(out)))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3])

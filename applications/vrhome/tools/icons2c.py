#!/usr/bin/env python3
# bake art/icons/*.svg (Material Symbols, Apache-2.0 - see art/icons/NOTICE)
# into the alpha atlas the hud uploads as a texture. `make` regenerates
# out/gen/icon_ids.h (the GlyphId enum, sorted by filename) and
# out/gen/icons.inc (the atlas bytes + per-glyph uv rects).
import glob
import math
import os
import re
import sys

CELL = 96      # px per glyph in the atlas
SS = 4         # supersample factor for the edges
PAD = 2        # transparent ring around each cell
COLS = 6

TOKEN = re.compile(
    r'[AaCcHhLlMmQqSsTtVvZz]|[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?')


def flatten(p0, p1, p2, p3=None, n=24):
    pts = []
    for i in range(1, n + 1):
        t = i / n
        mt = 1.0 - t
        if p3 is None:
            x = mt * mt * p0[0] + 2 * mt * t * p1[0] + t * t * p2[0]
            y = mt * mt * p0[1] + 2 * mt * t * p1[1] + t * t * p2[1]
        else:
            x = (mt**3 * p0[0] + 3 * mt * mt * t * p1[0] +
                 3 * mt * t * t * p2[0] + t**3 * p3[0])
            y = (mt**3 * p0[1] + 3 * mt * mt * t * p1[1] +
                 3 * mt * t * t * p2[1] + t**3 * p3[1])
        pts.append((x, y))
    return pts


def arc_points(x0, y0, rx, ry, rot, large, sweep, x1, y1, n=24):
    # svg endpoint -> center parameterization, then straight segments
    phi = math.radians(rot)
    cp, sp = math.cos(phi), math.sin(phi)
    dx, dy = (x0 - x1) * 0.5, (y0 - y1) * 0.5
    xp, yp = cp * dx + sp * dy, -sp * dx + cp * dy
    rx, ry = abs(rx), abs(ry)
    lam = xp * xp / (rx * rx) + yp * yp / (ry * ry)
    if lam > 1.0:
        s = math.sqrt(lam)
        rx, ry = rx * s, ry * s
    num = rx * rx * ry * ry - rx * rx * yp * yp - ry * ry * xp * xp
    den = rx * rx * yp * yp + ry * ry * xp * xp
    coef = math.sqrt(max(0.0, num / den))
    if large == sweep:
        coef = -coef
    cxp, cyp = coef * rx * yp / ry, -coef * ry * xp / rx
    cx = cp * cxp - sp * cyp + (x0 + x1) * 0.5
    cy = sp * cxp + cp * cyp + (y0 + y1) * 0.5

    def ang(ux, uy, vx, vy):
        d = math.hypot(ux, uy) * math.hypot(vx, vy)
        c = max(-1.0, min(1.0, (ux * vx + uy * vy) / d))
        a = math.acos(c)
        return -a if ux * vy - uy * vx < 0 else a

    th0 = ang(1, 0, (xp - cxp) / rx, (yp - cyp) / ry)
    dth = ang((xp - cxp) / rx, (yp - cyp) / ry,
              (-xp - cxp) / rx, (-yp - cyp) / ry)
    if not sweep and dth > 0:
        dth -= 2 * math.pi
    elif sweep and dth < 0:
        dth += 2 * math.pi
    pts = []
    for i in range(1, n + 1):
        t = th0 + dth * i / n
        pts.append((cx + rx * cp * math.cos(t) - ry * sp * math.sin(t),
                    cy + rx * sp * math.cos(t) + ry * cp * math.sin(t)))
    return pts


def parse_path(d):
    toks = TOKEN.findall(d)
    subs, cur = [], None
    x = y = sx = sy = 0.0
    cxp = cyp = qxp = qyp = None
    i, cmd = 0, None
    while i < len(toks):
        if re.match(r'[A-Za-z]', toks[i]):
            cmd, i = toks[i], i + 1
        c, rel = cmd.upper(), cmd.islower()
        if c == 'Z':
            x, y = sx, sy
            continue
        if c == 'M':
            nx, ny = float(toks[i]), float(toks[i + 1])
            i += 2
            if rel:
                nx, ny = nx + x, ny + y
            x, y = sx, sy = nx, ny
            cur = [(x, y)]
            subs.append(cur)
            cmd = 'l' if rel else 'L'
            cxp = cyp = qxp = qyp = None
            continue
        if cur is None:
            raise ValueError('path data starts without a moveto')
        pts = []
        if c == 'L':
            nx, ny = float(toks[i]), float(toks[i + 1])
            i += 2
            pts.append((nx + x, ny + y) if rel else (nx, ny))
        elif c == 'H':
            nx = float(toks[i])
            i += 1
            pts.append((nx + x if rel else nx, y))
        elif c == 'V':
            ny = float(toks[i])
            i += 1
            pts.append((x, ny + y if rel else ny))
        elif c in 'CS':
            if c == 'C':
                x1, y1 = float(toks[i]), float(toks[i + 1])
                i += 2
                x1, y1 = (x1 + x, y1 + y) if rel else (x1, y1)
            else:
                x1, y1 = ((2 * x - cxp, 2 * y - cyp)
                          if cxp is not None else (x, y))
            x2, y2, nx, ny = (float(toks[i]), float(toks[i + 1]),
                              float(toks[i + 2]), float(toks[i + 3]))
            i += 4
            if rel:
                x2, y2, nx, ny = x2 + x, y2 + y, nx + x, ny + y
            pts = flatten((x, y), (x1, y1), (x2, y2), (nx, ny))
            cxp, cyp = x2, y2
            qxp = qyp = None
        elif c in 'QT':
            if c == 'Q':
                x1, y1 = float(toks[i]), float(toks[i + 1])
                i += 2
                x1, y1 = (x1 + x, y1 + y) if rel else (x1, y1)
            else:
                x1, y1 = ((2 * x - qxp, 2 * y - qyp)
                          if qxp is not None else (x, y))
            nx, ny = float(toks[i]), float(toks[i + 1])
            i += 2
            if rel:
                nx, ny = nx + x, ny + y
            pts = flatten((x, y), (x1, y1), (nx, ny))
            qxp, qyp = x1, y1
            cxp = cyp = None
        elif c == 'A':
            rx, ry, rot, large, sweep, nx, ny = (
                float(toks[i]), float(toks[i + 1]), float(toks[i + 2]),
                float(toks[i + 3]), float(toks[i + 4]),
                float(toks[i + 5]), float(toks[i + 6]))
            i += 7
            if rel:
                nx, ny = nx + x, ny + y
            pts = arc_points(x, y, rx, ry, rot, large, sweep, nx, ny)
            cxp = cyp = qxp = qyp = None
        else:
            raise ValueError('unhandled path command %r' % cmd)
        if c not in 'CS':
            cxp = cyp = None
        if c not in 'QT':
            qxp = qyp = None
        cur.extend(pts)
        x, y = cur[-1]
    return subs


def rasterize(subs, box):
    minx, miny, w, h = box
    n = CELL * SS
    img = bytearray(n * n)
    edges = []
    for poly in subs:
        for k in range(len(poly)):
            x0, y0 = poly[k]
            x1, y1 = poly[(k + 1) % len(poly)]
            if y0 != y1:
                edges.append((x0, y0, x1, y1))
    for row in range(n):
        wy = miny + (row + 0.5) * h / n
        xs = []
        for x0, y0, x1, y1 in edges:
            lo, hi = (y0, y1) if y0 < y1 else (y1, y0)
            if wy < lo or wy >= hi:
                continue
            xs.append((x0 + (wy - y0) * (x1 - x0) / (y1 - y0),
                       1 if y1 > y0 else -1))
        xs.sort()
        wn = 0
        for k in range(len(xs) - 1):
            wn += xs[k][1]
            if wn == 0:
                continue
            fa = (xs[k][0] - minx) * n / w
            fb = (xs[k + 1][0] - minx) * n / w
            for col in range(max(0, math.ceil(fa - 0.5)),
                             min(n, math.ceil(fb - 0.5))):
                img[row * n + col] = 1
    out = bytearray(CELL * CELL)
    for y in range(CELL):
        for x in range(CELL):
            acc = 0
            for sy in range(SS):
                base = (y * SS + sy) * n + x * SS
                acc += sum(img[base:base + SS])
            out[y * CELL + x] = acc * 255 // (SS * SS)
    return out


def main():
    src_dir, gen_dir = sys.argv[1], sys.argv[2]
    files = sorted(glob.glob(os.path.join(src_dir, '*.svg')))
    if not files:
        sys.exit('no svgs under %s' % src_dir)
    names = [os.path.splitext(os.path.basename(f))[0] for f in files]

    stride = CELL + PAD * 2
    cols = COLS
    rows = (len(files) + cols - 1) // cols
    aw, ah = cols * stride, rows * stride
    atlas = bytearray(aw * ah)
    for idx, path in enumerate(files):
        text = open(path).read()
        vb = [float(v) for v in re.search(
            r'viewBox="([\d.\-\s]+)"', text).group(1).split()]
        polys = []
        for d in re.findall(r'<path[^>]*d="([^"]+)"', text):
            polys += parse_path(d)
        cell = rasterize(polys, vb)
        ox, oy = (idx % cols) * stride + PAD, (idx // cols) * stride + PAD
        for y in range(CELL):
            dst = (oy + y) * aw + ox
            atlas[dst:dst + CELL] = cell[y * CELL:(y + 1) * CELL]

    os.makedirs(gen_dir, exist_ok=True)
    with open(os.path.join(gen_dir, 'icon_ids.h'), 'w') as f:
        f.write('// generated by tools/icons2c.py from art/icons - the\n'
                '// Material Symbols set the dash draws, sorted by filename\n'
                '#pragma once\n\nenum GlyphId {\n')
        for nm in names:
            f.write('    GI_%s,\n' % nm.upper())
        f.write('    GI_COUNT\n};\n')

    with open(os.path.join(gen_dir, 'icons.inc'), 'w') as f:
        f.write('// generated by tools/icons2c.py from art/icons - Material\n'
                '// Symbols alpha atlas, CELL=%d PAD=%d\n' % (CELL, PAD))
        f.write('static const int kIconAtlasW = %d;\n' % aw)
        f.write('static const int kIconAtlasH = %d;\n' % ah)
        f.write('static const unsigned char kIconAtlas[] =\n')
        for y in range(ah):
            f.write('    "%s"\n' % ''.join(
                '\\x%02x' % b for b in atlas[y * aw:(y + 1) * aw]))
        f.write('    ;\n')
        f.write('// u0,v0 .. u1,v1 of each glyph\'s cell, in GlyphId order\n')
        f.write('static const float kIconUV[][4] = {\n')
        for idx in range(len(files)):
            x0 = ((idx % cols) * stride + PAD) / aw
            y0 = ((idx // cols) * stride + PAD) / ah
            x1 = ((idx % cols) * stride + PAD + CELL) / aw
            y1 = ((idx // cols) * stride + PAD + CELL) / ah
            f.write('    {%.6ff, %.6ff, %.6ff, %.6ff},\n' % (x0, y0, x1, y1))
        f.write('};\n')


if __name__ == '__main__':
    main()

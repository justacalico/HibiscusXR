#!/usr/bin/env python3
# Renders the boot media that ships in the image. Run from anywhere; outputs
# land back in the repo:
#
#   system/overlay/media/bootanimation/   desc.txt + part0/part1 frames, packed
#       into /system/media/bootanimation.zip by tools/build/143_build_image2.sh
#   system/fullstage/media/LoadingRes/    VRShell's 2D loading screen art, staged
#       to /system/media/LoadingRes via the stub image (dist/make-loadingres-img.sh)
#
# Needs rsvg-convert and pillow on the host. CI never runs this - the PNGs are
# committed, this is just how they were made.
#
# Panel notes (why the frames look like this): the display is a single
# 3840x2160 landscape panel split between the lenses - each eye sees one
# 1920x2160 half, so anything centred on the framebuffer lands on the seam and
# is clipped by both lenses. Every frame therefore carries the mark twice, at
# the two eye centres, so each eye gets a complete logo. bootanimation draws
# frames unscaled, centred inside the desc rectangle, which is itself centred
# on the screen - a 2480x560 band centred covers both eye centres exactly.

import math
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))

MARK_SVG = os.path.join(HERE, "hibiscus-mark.svg")
BOOTANIM_DIR = os.path.join(ROOT, "system/overlay/media/bootanimation")
LOADING_DIR = os.path.join(ROOT, "system/fullstage/media/LoadingRes")

# svg-space constants (hibiscus-mark.svg viewBox is 512x512)
CENTRE = (256.0, 264.0)
ORBIT_RX, ORBIT_RY, ORBIT_TILT = 192.0, 64.0, math.radians(-25.0)
DOT_R, HALO_R = 13.0, 24.0
CYAN = (94, 231, 255)

# bootanim band: desc is 2480x560, centred on the 3840x2160 framebuffer, so it
# lands at (680,800). Eye centres sit at band x=280 and x=2200 -> 960 and 2880.
BW, BH = 2480, 560
FPS = 30
INTRO_FRAMES = 8          # part0, plays once
LOOP_FRAMES = 30          # part1, orbits until the system takes over
LOGO_H = 420              # px, inside the 560px band

def render_mark(png_path, width):
    subprocess.run(
        ["rsvg-convert", "-w", str(width), MARK_SVG, "-o", png_path], check=True)


def load_mark(path, target_h):
    """Raster -> tight crop -> resized. Returns (img, units->px mapper)."""
    img = Image.open(path).convert("RGBA")
    bbox = img.getbbox()
    px_per_unit = img.width / 512.0
    img = img.crop(bbox)
    scale = target_h / img.height
    img = img.resize((round(img.width * scale), target_h), Image.LANCZOS)
    k = px_per_unit * scale
    ox, oy = bbox[0], bbox[1]

    def to_px(x, y):
        return ((x * px_per_unit - ox) * scale, (y * px_per_unit - oy) * scale)

    return img, to_px, k


def orbit_pos(t, to_px):
    """Point on the tilted orbit ring at parameter t, in rendered pixels."""
    x = ORBIT_RX * math.cos(t)
    y = ORBIT_RY * math.sin(t)
    c, s = math.cos(ORBIT_TILT), math.sin(ORBIT_TILT)
    # svg rotate(-25) in a y-down frame; same handedness as the image
    return to_px(CENTRE[0] + x * c + y * s, CENTRE[1] - x * s + y * c)


def draw_dot(layer, cx, cy, r_core, r_halo):
    d = ImageDraw.Draw(layer)
    d.ellipse([cx - r_halo, cy - r_halo, cx + r_halo, cy + r_halo],
              fill=CYAN + (64,))
    d.ellipse([cx - r_core, cy - r_core, cx + r_core, cy + r_core],
              fill=CYAN + (255,))


def glow_blob(size, radius, alpha):
    blob = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(blob)
    d.ellipse([size / 2 - radius, size / 2 - radius,
               size / 2 + radius, size / 2 + radius],
              fill=(255, 79, 139, alpha))
    return blob.filter(ImageFilter.GaussianBlur(radius * 0.6))


def eye_tile(mark, to_px, k, t, pulse):
    """One 560x560 eye view: glow, mark, orbiting dot."""
    tile = Image.new("RGBA", (560, 560), (0, 0, 0, 0))
    sc = 1.0 + 0.02 * math.sin(pulse)
    w = round(mark.width * sc)
    h = round(mark.height * sc)
    m = mark.resize((w, h), Image.LANCZOS)
    mx, my = 280 - w / 2, 280 - h / 2
    glow_a = round(60 + 40 * (0.5 + 0.5 * math.sin(pulse)))
    tile.alpha_composite(glow_blob(560, round(170 * sc), glow_a))
    tile.alpha_composite(m, (round(mx), round(my)))
    dx, dy = orbit_pos(t, to_px)
    draw_dot(tile, mx + dx * sc, my + dy * sc, DOT_R * k * sc, HALO_R * k * sc)
    return tile


def band(tile, brightness=1.0):
    f = Image.new("RGB", (BW, BH), (0, 0, 0))
    f.paste(tile, (0, 0), tile)
    f.paste(tile, (1920, 0), tile)
    if brightness != 1.0:
        f = ImageEnhance.Brightness(f).enhance(brightness)
    return f


def save(img, *parts):
    path = os.path.join(*parts)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path, optimize=True)
    return path


def bootanimation(mark, to_px, k):
    # intro: fade + settle, dot parked at its resting spot on the ring
    for i in range(INTRO_FRAMES):
        e = 1.0 - (1.0 - i / (INTRO_FRAMES - 1)) ** 3  # ease-out cubic
        tile = eye_tile(mark, to_px, k, 0.0, 0.0)
        if e < 1.0:
            w, h = tile.size
            s = 0.9 + 0.1 * e
            tile = tile.resize((round(w * s), round(h * s)), Image.LANCZOS)
            pad = Image.new("RGBA", (560, 560), (0, 0, 0, 0))
            pad.alpha_composite(tile, (round(280 - tile.width / 2),
                                       round(280 - tile.height / 2)))
            tile = pad
        save(band(tile, e), BOOTANIM_DIR, "part0", f"{i:05d}.png")

    # loop: the tracking dot rides the orbit once per second, mark breathes
    for i in range(LOOP_FRAMES):
        t = 2 * math.pi * i / LOOP_FRAMES
        tile = eye_tile(mark, to_px, k, t, t)
        save(band(tile), BOOTANIM_DIR, "part1", f"{i:05d}.png")

    with open(os.path.join(BOOTANIM_DIR, "desc.txt"), "w") as f:
        f.write(f"{BW} {BH} {FPS}\n")
        f.write("c 1 0 part0\n")
        f.write("p 0 0 part1\n")


def loadingres(big, mark):
    # the panel VRShell draws the loading screen on; dark gradient, centred mark
    bg = Image.new("RGB", (1080, 720))
    top, bot = (26, 18, 54), (15, 8, 25)
    px = bg.load()
    for y in range(720):
        u = y / 719
        row = tuple(round(a + (b - a) * u) for a, b in zip(top, bot))
        for x in range(1080):
            px[x, y] = row
    glow = glow_blob(720, 230, 46)
    bg.paste(glow, (180, 0), glow)
    h = 320
    w = round(mark.width * h / mark.height)
    m = mark.resize((w, h), Image.LANCZOS)
    bg.paste(m, (540 - w // 2, 340 - h // 2), m)
    save(bg, LOADING_DIR, "inside_background_img.png")

    # spinner: the mark's orbit ring with the dot running it
    small, to_px2, k2 = load_mark(big, 150)
    for i in range(24):
        t = 2 * math.pi * i / 24
        fr = Image.new("RGBA", (200, 200), (0, 0, 0, 0))
        ox, oy = 100 - small.width // 2, 100 - small.height // 2
        fr.alpha_composite(small, (ox, oy))
        dx, dy = orbit_pos(t, to_px2)
        draw_dot(fr, ox + dx, oy + dy, DOT_R * k2, HALO_R * k2)
        save(fr, LOADING_DIR, "img", f"loading_animation_{i:05d}.png")


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "loadingres":
        which = ("loadingres",)
    else:
        which = ("bootanim", "loadingres")
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
        big = tmp.name
    render_mark(big, 2048)
    mark, to_px, k = load_mark(big, LOGO_H)
    print(f"mark: {mark.width}x{mark.height}px ({k:.3f} px/unit)")
    if "bootanim" in which:
        bootanimation(mark, to_px, k)
    if "loadingres" in which:
        loadingres(big, mark)
    os.unlink(big)


if __name__ == "__main__":
    main()

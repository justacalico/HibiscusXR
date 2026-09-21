#!/usr/bin/env python3
"""Draw the launcher icon: three slider tracks with knobs on the panel
gradient. Writes adaptive-icon foreground PNGs and legacy PNGs into
android/app/src/main/res. Run from the repo root: python3 tool/make_icon.py
"""
from PIL import Image, ImageDraw

# panel palette, same as lib/src/ui/theme.dart
BG_TOP = (38, 49, 61)
BG_BOTTOM = (24, 31, 40)
TRACK = (154, 167, 180)
ACCENT = (78, 156, 255)
KNOB = (242, 245, 248)

DENSITIES = {
    "mdpi": (48, 108),
    "hdpi": (72, 162),
    "xhdpi": (96, 216),
    "xxhdpi": (144, 324),
    "xxxhdpi": (192, 432),
}

# rows: (y fraction, knob x fraction)
ROWS = [(0.32, 0.62), (0.50, 0.30), (0.68, 0.75)]
TRACK_L, TRACK_R = 0.22, 0.78


def gradient(size):
    img = Image.new("RGBA", (size, size))
    for y in range(size):
        t = y / size
        img.paste(
            tuple(int(a + (b - a) * t) for a, b in zip(BG_TOP, BG_BOTTOM)),
            (0, y, size, y + 1),
        )
    return img


def draw_sliders(draw, size, inset):
    """Slider rows centered inside `inset` (the adaptive safe zone)."""
    track_w = size * 0.045
    for yf, kx in ROWS:
        y = size * yf
        x0, x1 = size * TRACK_L + inset * 0, size * TRACK_R
        # keep the glyph inside the center 66% circle-safe zone
        draw.rounded_rectangle(
            [x0, y - track_w / 2, x1, y + track_w / 2],
            radius=track_w / 2,
            fill=TRACK + (160,),
        )
        kxr = x0 + (x1 - x0) * kx
        r = size * 0.052
        draw.ellipse([kxr - r, y - r, kxr + r, y + r], fill=KNOB)
        ri = r * 0.52
        draw.ellipse([kxr - ri, y - ri, kxr + ri, y + ri], fill=ACCENT)


def rounded_mask(size, radius):
    m = Image.new("L", (size, size), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    return m


def make(kind, size):
    ss = size * 4
    if kind == "foreground":
        img = Image.new("RGBA", (ss, ss), (0, 0, 0, 0))
        draw_sliders(ImageDraw.Draw(img), ss, ss * 0.17)
    else:
        img = gradient(ss)
        draw_sliders(ImageDraw.Draw(img), ss, 0)
        # legacy icons get the squircle baked in
        img.putalpha(rounded_mask(ss, ss * 0.24))
    return img.resize((size, size), Image.LANCZOS)


for name, (legacy, fg) in DENSITIES.items():
    make("foreground", fg).save(
        f"android/app/src/main/res/mipmap-{name}/ic_launcher_foreground.png"
    )
    make("legacy", legacy).save(
        f"android/app/src/main/res/mipmap-{name}/ic_launcher.png"
    )
    print(name, "done")

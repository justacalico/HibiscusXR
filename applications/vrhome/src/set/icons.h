#pragma once

#include "../math/mat4.h"

struct HudEngine;

// the sidebar section glyph for SetSec: s is the icon's half-size in
// card-local metres, col the glyph colour
void setSecIcon(HudEngine* e, const Mat4& vp, int sec,
                const float ic[3], const float r[3], const float up[3],
                float s, const float col[4]);

// small row glyphs: chevron (action rows), lock (secured nets), check
// (active pick), trash (env delete), refresh (rescan), battery
enum SetGlyph {
    SG_CHEVRON = 0, SG_LOCK, SG_CHECK, SG_TRASH, SG_REFRESH, SG_BATT,
};
void setGlyph(HudEngine* e, const Mat4& vp, int glyph,
              const float ic[3], const float r[3], const float up[3],
              float s, const float col[4]);

// the project mark on the About card's brand row: five petals around a
// dark throat, the gold stamen ray and the tracking orbit ring with its
// planet - the procedural twin of the old app's hibiscusxr_icon.svg,
// fixed brand colours instead of the theme palette. s is half-size
void setBrandMark(HudEngine* e, const Mat4& vp, const float ic[3],
                  const float r[3], const float up[3], float s,
                  float alpha);

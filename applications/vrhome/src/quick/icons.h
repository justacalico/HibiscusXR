#pragma once

#include "../math/mat4.h"

struct HudEngine;

// tile glyph for QsId id: s is the icon's half-size in card-local metres,
// col the glyph colour, back the tile's fill (the crescent bites a
// card-coloured disc out of the moon)
void quickIcon(HudEngine* e, const Mat4& vp, int id, const float ic[3],
               const float r[3], const float up[3], float s,
               const float col[4], const float back[4]);

// the slider row icons: sun for brightness, speaker for volume
void quickSliderIcon(HudEngine* e, const Mat4& vp, int slider,
                     const float ic[3], const float r[3],
                     const float up[3], float s, const float col[4]);

// the title row's settings gear
void quickGearIcon(HudEngine* e, const Mat4& vp, const float ic[3],
                   const float r[3], const float up[3], float s,
                   const float col[4]);

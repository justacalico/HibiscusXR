#pragma once

#include "icon_ids.h"

// the baked Material Symbols atlas tools/icons2c.py regenerates into
// out/gen/icons.inc - pure data + lookup so the host tests can reach it
int iconAtlasW();
int iconAtlasH();
const unsigned char* iconAtlas();

// the glyph's cell in atlas uvs: u0,v0 top-left, u1,v1 bottom-right.
// false when id is out of range
bool iconUv(int id, float uv[4]);

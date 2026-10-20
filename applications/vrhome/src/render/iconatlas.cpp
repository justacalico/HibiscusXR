#include "iconatlas.h"

#include <cstring>

#include "icons.inc"

int iconAtlasW() { return kIconAtlasW; }
int iconAtlasH() { return kIconAtlasH; }
const unsigned char* iconAtlas() { return kIconAtlas; }

bool iconUv(int id, float uv[4]) {
    if (id < 0 || id >= GI_COUNT) return false;
    memcpy(uv, kIconUV[id], 4 * sizeof(float));
    return true;
}

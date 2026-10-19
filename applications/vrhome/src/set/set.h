#pragma once

#include "../math/mat4.h"

struct HudEngine;

// The settings overlay: the full settings app lives inside the dash as a
// card on the ring. The quick panel's gear and About tile open it; the
// platform side rides the backend's settingsStatus pull and the
// settings* write calls.

// open the card on a section (SetSec); closes the grid and quick panel
void setOpen(HudEngine* e, int section);
void setClose(HudEngine* e);

// render thread: rebuild rows on a section/snap change and pull the
// platform snapshot while the card is up
void syncSet(HudEngine* e);

// confirm press landed on the card: record the zone, arm sliders/drags
void setPress(HudEngine* e);

// release after a setPress: fires the zone's action when the release
// landed back on the same spot
void setRelease(HudEngine* e);

// held-press tracking: slider drags and the content scroll
void setDragTick(HudEngine* e);

// wheel input while the card is up
void setScroll(HudEngine* e, float delta);

// the card, drawn after the grid/quick so the dialog layers over them
void drawSet(HudEngine* e, const Mat4& viewProj);

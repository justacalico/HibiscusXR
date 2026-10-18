#pragma once

#include "../math/mat4.h"

struct HudEngine;

// The quick panel: the settings card inside the dash. The dock's status
// pill toggles it; toggles and sliders ride QuickState in the host
// process through the backend.

// render thread: pull the platform snapshot while the card is up
void syncQuick(HudEngine* e);

// release on a tile: toggles flip through the backend, actions fire their
// dash-side effect (recenter re-anchors, About opens the settings app)
void quickActivate(HudEngine* e, int idx);

// release on the title row's gear: open the settings app on a window
void quickGear(HudEngine* e);

// a slider row's current value driven by a card-local u, live through the
// backend while the drag runs
void quickSliderTo(HudEngine* e, int slider, float u);

// the card, drawn after the grid so the two never z-fight at their
// shared distance
void drawQuick(HudEngine* e, const Mat4& viewProj);

#pragma once

#include "../math/mat4.h"

struct HudEngine;

// The app-grid overlay: the library lives inside the dash now. The card
// hangs in front of the window slots and scrolls its cells inside a
// band under the title row.

// render thread: rebuild the cell list when the java package set changed
void syncGrid(HudEngine* e);

// launch the pressed cell's app and drop the overlay
void gridActivate(HudEngine* e, int idx);

// draw the card, the title row and the (scrolled) cells
void drawGrid(HudEngine* e, const Mat4& viewProj);

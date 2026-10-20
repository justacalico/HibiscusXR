#pragma once

#include "../math/mat4.h"

struct HudEngine;

// The app-grid overlay: the library lives inside the dash now. The card
// hangs in front of the window slots and scrolls its cells inside a
// band under the title row.

// render thread: pull the app list + groups when they changed and rebuild
// the visible cells
void syncGrid(HudEngine* e);

// launch the pressed cell's app and drop the overlay
void gridActivate(HudEngine* e, int idx);

// open a cell's context menu at its dots affordance
void gridMenuOpen(HudEngine* e, int idx);

// a context-menu row fired: pin toggle, a group's membership toggle, or
// the new-group row which also adopts the menu's app
void gridMenuActivate(HudEngine* e, int row);

// a dropdown row fired: create a group, pick "all apps", or filter to one
void gridDropActivate(HudEngine* e, int row);

// draw the card, the title row, the (scrolled) cells and any open popup
void drawGrid(HudEngine* e, const Mat4& viewProj);

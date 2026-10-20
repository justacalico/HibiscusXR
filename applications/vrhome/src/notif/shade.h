#pragma once

#include "../math/mat4.h"

struct HudEngine;

// The notification shade: the card the dock's bell pill opens. Every
// live shade record lists as a row - icon, title, body, a dismiss badge
// on the clearable ones - with a clear-all pill and a close disc on the
// title row. Row ordering, geometry and hit policy live in layout.cpp so
// they run on the host; this file is the impure half: text measurement,
// textures and the backend's dismiss calls.

// per-frame, render thread: keep the clear-all pill sized to its label
// and the scroll in range while the card is up
void syncShade(HudEngine* e);

// a row's dismiss badge: cancel that notification through the listener
void shadeDismissRow(HudEngine* e, int idx);

// the title row's clear-all pill: cancel every clearable notification
void shadeClearAll(HudEngine* e);

// the card, drawn with the other dash overlays
void drawShade(HudEngine* e, const Mat4& vp);

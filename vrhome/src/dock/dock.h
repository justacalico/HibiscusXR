#pragma once

#include "../math/mat4.h"

struct HudEngine;

// Dock glue: bridge pulls, icon textures and actions - the impure half of
// the dock; ordering/layout/hit-test policy lives in layout.cpp so it can
// run on the host.

// per-frame, render thread: pull the pin list + immersive task set from the
// bridge, rebuild the item list, resolve icons and labels for new entries
void syncDock(HudEngine* e);

// per-frame: the confirm-hold pin gesture on a dock item; fires once the
// press passes kDockPinMs, dockPinP carries the 0..1 fill for the ring
void dockTick(HudEngine* e);

// release on an item: focus/restore a running app, launch a cold one
void dockActivate(HudEngine* e, int idx);

// the close badge on a live immersive item: kill its task
void dockClose(HudEngine* e, int idx);

// toggle pkg in the pin list and persist it through the bridge
void dockTogglePin(HudEngine* e, const char* pkg);

// push the current pin list to the java side for persistence
void dockPushPins(HudEngine* e);

// the strip itself, drawn after the panels so it layers on the dash front
void drawDock(HudEngine* e, const Mat4& vp);

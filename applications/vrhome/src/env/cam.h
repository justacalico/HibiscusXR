#pragma once

struct Engine;

// Passthrough camera glue: the tracking pair behind device "0" streams a
// combined 1280x400 frame into an external texture. Everything is driven
// from the render loop - ptTick manages the session's lifecycle, ptUpdate
// latches the newest frame.
void ptTick(Engine* e);
void ptUpdate(Engine* e);
void ptStop(Engine* e);

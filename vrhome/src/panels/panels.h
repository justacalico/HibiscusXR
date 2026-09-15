#pragma once

struct Engine;

// create a GL texture + virtual display + panel record. taskId stays -1 for
// launcher/explicit launches. Returns panels index or -1.
int  openPanel(Engine* e, float yaw, float pitch);

void closePanel(Engine* e, int idx);

// drop the oldest app window when the ring is full; the library panel is the
// shell's launcher and never gets evicted. returns false if nothing could go
bool evictOldestApp(Engine* e);

// pull the newest frame of each virtual display into its texture
void updatePanels(Engine* e);

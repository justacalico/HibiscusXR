#pragma once

struct Engine;
struct ANativeWindow;

// window surface + EGL lifecycle. The context outlives any single window
// surface so the compositor can keep running (panel adoption, texture
// updates) while the surface is gone. `win` is the activity window for the
// env or the overlay SurfaceView's surface for the HUD.
int  initEglContext(Engine* e);   // display+context+pbuffer, once per process
int  initWindow(Engine* e, ANativeWindow* win);
void termWindow(Engine* e);
void termDisplay(Engine* e);

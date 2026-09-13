#pragma once

struct Engine;

// window surface + EGL lifecycle. The context outlives any single window
// surface so the compositor can keep running (panel adoption, texture
// updates) while a stray fullscreen app covers the physical display.
int  initWindow(Engine* e);
void termWindow(Engine* e);
void termDisplay(Engine* e);

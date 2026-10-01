#pragma once

struct HudEngine;

// adb-driven test hooks: each debug.vrhome.* prop fires once per new
// value, so a shell can drive launches, taps and dock gestures while the
// HUD runs headless
void runDebugHooks(HudEngine* e);

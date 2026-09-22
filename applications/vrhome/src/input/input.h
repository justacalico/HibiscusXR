#pragma once

struct HudEngine;
struct Mat4;

// one key press from the HUD window's dispatchKeyEvent. code/action/repeat
// match android.view.KeyEvent values
void hudKey(HudEngine* e, int code, int action, int repeat);

// per-frame while the confirm button is held: inject drag MOVEs along the
// aim ray (gaze or controller, the caller picks)
void dragTick(HudEngine* e, const float o[3], const float d[3]);

// per-frame while a drag handle is held: swing the whole panel ring with the
// gaze so every window moves together
void moveTick(HudEngine* e);

#pragma once

struct HudEngine;
struct Mat4;

// one key press from the HUD window's dispatchKeyEvent. code/action/repeat
// match android.view.KeyEvent values
void hudKey(HudEngine* e, int code, int action, int repeat);

// per-frame while the confirm button is held: inject drag MOVEs at the gaze
void dragTick(HudEngine* e, const Mat4& head);

// per-frame while a drag handle is held: swing the whole panel ring with the
// gaze so every window moves together
void moveTick(HudEngine* e);

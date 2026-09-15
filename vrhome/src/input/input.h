#pragma once

#include <android_native_app_glue.h>
#include <android/input.h>

// the glue drains the input queue itself and calls this per event - polling
// the queue manually finds it already empty, so input MUST be handled here
int32_t onInputEvent(android_app* app, AInputEvent* ev);

struct Engine;
struct Mat4;

// per-frame while the confirm button is held: inject drag MOVEs at the gaze
void dragTick(Engine* e, const Mat4& head);

// per-frame while a drag handle is held: swing the whole panel ring with the
// gaze yaw so every window moves together
void moveTick(Engine* e);

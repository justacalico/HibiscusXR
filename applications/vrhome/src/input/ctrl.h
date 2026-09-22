#pragma once

struct HudEngine;
struct Mat4;

// per-frame controller update on the render thread: snapshot the shared
// memory, refresh connection/active state, update the aim ray and queue
// button edges for routing after the pick. All glue - decisions live in
// input_state.cpp and aim.cpp.
void ctrlTick(HudEngine* e, const Mat4& head, float sensRoll, float worldX,
              float roll, long long nowMs);

// send queued controller events through the same paths real keys take;
// runs after the pick so a press lands on fresh hover state
void ctrlFlush(HudEngine* e);

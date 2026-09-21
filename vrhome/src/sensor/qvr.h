#pragma once

struct Engine;

// Poll the qvrservice head pose and refresh e->headPos/headPosValid.
// Lazily connects on first call; a no-op when the service or client
// library is unavailable. Thin glue - the frame math lives in math/head.cpp.
void qvrPoll(Engine* e);

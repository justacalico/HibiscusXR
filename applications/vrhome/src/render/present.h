#pragma once

// Present-rate accounting, pure so the unit tests can reach it.
//
// e->fps counts submitted frames, which on a free-running swap is the loop
// rate - anywhere from 52 to 72 on the dash while the panel is really only
// flipping a couple times a second. The display's real rate shows up in the
// timestamps the compositor stamps on each composite pass
// (eglGetCompositorTimingANDROID / EGL_ANDROID_get_frame_timestamps):
// frames submitted faster than the flip all share the same stamp, so
// distinct timestamps per second = real flips per second ("monado fps").
//
// Returns true when ns advances the stream: the first valid sample or a
// newer timestamp. Anything <= 0 or not newer than the last stamp is not a
// new present.
static inline bool presentTick(long long* lastNs, int* count, long long ns) {
    if (ns <= 0 || ns <= *lastNs) return false;
    *lastNs = ns;
    ++*count;
    return true;
}

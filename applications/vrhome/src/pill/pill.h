#pragma once

// The shared move pill: a short line centred under a quad that drags
// whatever it hangs from. The dock strip, the floating keyboard and
// floating windows all carry the same pill, so the drop rule and the hit
// shape live here once instead of being cloned per surface. Pure geometry
// - no GL - so the tests can reach it.

// how far under a quad's centre the pill's centre hangs, world units:
// the quad's half-height, the visual gap and the line's own half-thickness
float movePillDrop(float hh);

// is (u,v) - quad coords, -1..1 inside the quad - on the pill hanging
// under it. The pill lives below v=-1, so callers check this before the
// in-bounds test; gaze aim is coarse so the hit box pads the drawn line
bool onMovePill(float u, float v, float hw, float hh);

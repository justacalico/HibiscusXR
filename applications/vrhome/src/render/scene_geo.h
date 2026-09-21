#pragma once

#include <vector>

// Static scenery vertex builders: pos3+col3 interleaved floats. Pure data so
// the geometry is covered by the host tests; scene.cpp uploads + draws.

// floor grid: gives the eye something to lock onto so a "black" scene is
// never just empty space. returns vertex count, GL_LINES order
int buildGrid(std::vector<float>& out);

// sky dome: open cylinder with a vertical gradient so the scene has a
// horizon instead of a flat clear colour. returns vertex count, GL_TRIANGLES
int buildSky(std::vector<float>& out);

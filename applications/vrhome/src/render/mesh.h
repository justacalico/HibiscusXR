#pragma once

#include <cstddef>
#include <vector>

// flat triangle soup out of a Wavefront OBJ: xyz per vertex, three per face.
// Only v/f records are read; faces may be v, v/vt, v//vn or v/vt/vn, and
// polygons fan out. Pure - host tests feed it fixture text.
struct Mesh {
    std::vector<float> v;
};

bool meshFromObj(const char* text, size_t len, Mesh* out);

// centre the bbox on the origin and scale the longest axis to `size` -
// the source models are authored in arbitrary units, so the dash pins them
// to a real-world size instead of trusting the export
void meshFit(Mesh* m, float size);

#pragma once

// Passthrough warp meshes. The tracking pair delivers one 1280x400 frame
// with the two cameras' pixels in interleaved columns (even = camera 0),
// each camera's 640x400 image rotated 90 in place. Each eye gets a grid
// of view rays projected through that eye's fisheye intrinsics, so the
// sampled texel lands where the world actually is. The builder is pure
// maths - host tests can check every vertex.

struct CamIntr {
    float fx, fy, cx, cy;      // pixels inside the camera's 640x400 half
    float k1, k2, k3, k4;      // FISHEYE_4_PARAMETERS equidistant coeffs
    float rig[9];              // row-major camera->rig rotation
};

// Fills verts with cols*rows quads as triangles: each vertex is
// {x, y, 0, u, v} in NDC, matching the floatProg aPos/aUV layout.
// tanX/tanY are the render projection's edge tangents (tan(fov/2) scaled
// by aspect). swapEyes maps the left eye to the right camera half;
// flipU/flipV mirror the image inside its half; rollDeg rotates the view
// ray around the forward axis; rotQuarters rotates the projected pixel
// around the principal point in 90-degree steps - the sensors sit
// portrait in the frame, so each half's image needs an in-plane turn.
// Returns the vertex count (6 per cell), 0 on bad input.
int buildPtMesh(float* verts, int cap, int eye, const CamIntr cams[2],
                int cols, int rows, float tanX, float tanY,
                bool swapEyes, bool flipU, bool flipV, float rollDeg,
                int rotQuarters);

// Number of floats a mesh needs: cols*rows*6 vertices, 5 floats each.
inline int ptMeshFloats(int cols, int rows) { return cols * rows * 6 * 5; }

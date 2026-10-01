#pragma once

#include <cstddef>
#include <vector>

// What the env process draws behind the HUD chrome. The selection arrives
// as a string on persist.hibiscus.environment (debug.vrhome.env wins for
// tuning): empty or "passthrough" keeps the camera feed, "builtin" is the
// procedural sky+grid, anything else is an environment id naming a zip in
// kEnvDir.
enum EnvMode {
    kEnvPassthrough = 0,
    kEnvBuiltin = 1,
    kEnvCustom = 2,
};

// one environment's renderable geometry: pos3+col3 interleaved, three
// verts per face - the same vertex layout sceneProg draws for the sky
struct EnvMap {
    std::vector<float> v;
};

// selection string -> mode. Unknown non-reserved values mean "treat as an
// environment id" - validity is checked separately by envIdOk.
EnvMode envModeOf(const char* sel);

// environment ids become filenames, so the charset is locked down:
// [A-Za-z0-9._-], 1-64 chars, no dot-only or dotdot tricks
bool envIdOk(const char* id);

// <kEnvDir>/<id>.zip - false (out empty) when the id is unusable
bool envZipPath(const char* id, char* out, size_t outSize);

// Parse a home-environment map.obj into flat pos+color soup.
//
//   - `o`/`g` lines switch the current part; faces under a part named
//     SpawnUser are not drawn - its vertices' bounding box marks where the
//     user's floor goes: the map is shifted so the box's bottom-centre
//     lands at (0, kEnvFloorY, 0), matching the built-in grid plane
//   - `v x y z` may carry a vertex colour (`v x y z r g b`, 0..1); unlit
//     exports get a neutral base either way, and every face is baked with
//     a fixed lambert term so uncoloured maps still read as geometry
//   - without a SpawnUser part the whole map's bounds take over, so
//     author-sloppy packs still stand somewhere sane
//
// Returns false on empty geometry. Pure - tests feed it fixture text.
bool envFromObj(const char* text, size_t len, EnvMap* out);

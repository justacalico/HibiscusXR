#include "resources.h"

#include "icons.h"
#include "scene_geo.h"
#include "shaders.h"
#include "../engine.h"
#include "../common/log.h"
#include "../text/font.h"

#include <vector>

int initGlResources(Engine* e) {
    if (e->glInit) return 0;
    e->glInit = true;
    LOGI("renderer %s", glGetString(GL_RENDERER));
    e->sceneProg = linkProg(kSceneVS, kSceneFS);
    e->warpProg = linkProg(kWarpVS, kWarpFS);
    e->textProg = linkProg(kTextVS, kTextFS);
#ifdef VRHOME_DESKTOP
    e->floatProg = linkProg(kFloatVS, kFloat2DFS);
#else
    e->floatProg = linkProg(kFloatVS, kFloatFS);
#endif
    e->shapeProg = linkProg(kShapeVS, kShapeFS);
    e->holdProg = linkProg(kShapeVS, kHoldFS);
    e->iconProg = linkProg(kFloatVS, kIconFS);
    if (!e->sceneProg || !e->warpProg || !e->textProg || !e->floatProg ||
            !e->shapeProg || !e->holdProg || !e->iconProg)
        return -1;
    if (!loadFont(e)) LOGE("font load failed, HUD text disabled");
    loadGlyphs(e);
    glGenBuffers(1, &e->textVbo);
    glGenBuffers(1, &e->panelVbo);
    glGenBuffers(1, &e->quadVbo);
    glGenBuffers(1, &e->gridVbo);
    glGenBuffers(1, &e->skyVbo);
    const float quad[] = {-1,-1, 1,-1, -1,1,  1,-1, 1,1, -1,1};
    glBindBuffer(GL_ARRAY_BUFFER, e->quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    std::vector<float> verts;
    e->gridVerts = buildGrid(verts);
    glBindBuffer(GL_ARRAY_BUFFER, e->gridVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);
    e->skyVerts = buildSky(verts);
    glBindBuffer(GL_ARRAY_BUFFER, e->skyVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);
    glEnable(GL_DEPTH_TEST);
    return 0;
}

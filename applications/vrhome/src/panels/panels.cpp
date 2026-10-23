#include "panels.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../common/log.h"
#include "../common/config.h"

#include <GLES2/gl2.h>
#include <cstring>
#include <ctime>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

static void initTex(unsigned target, unsigned* tex) {
    glGenTextures(1, tex);
    glBindTexture(target, *tex);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

int openPanel(HudEngine* e, float yaw, float pitch) {
    if (!e->backend || (int)e->panels.size() >= kMaxPanelRecs) return -1;
    Panel p;
    p.externalTex = e->backend->externalPanelTextures();
    initTex(p.externalTex ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D, &p.tex);
    if (e->backend->createPanel(&p, p.tex, kVdW, kVdH, kVdDpi) < 0) {
        glDeleteTextures(1, &p.tex);
        return -1;
    }
    memset(p.stMat, 0, sizeof(p.stMat));
    p.yaw = yaw;
    p.pitch = pitch;
    p.dist = e->dashDist;
    p.grabYaw = yaw;
    p.grabPitch = pitch;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    p.bornMs = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    e->panels.push_back(p);
    LOGI("panel %d on display %d yaw %.2f", (int)e->panels.size() - 1,
         p.displayId, yaw);
    return (int)e->panels.size() - 1;
}

void closePanel(HudEngine* e, int idx) {
    Panel& p = e->panels[idx];
    if (e->backend) e->backend->releasePanel(&p);
    if (p.tex) { GLuint t = p.tex; glDeleteTextures(1, &t); }
    e->panels.erase(e->panels.begin() + idx);
    if (e->hover == idx) { e->hover = -1; e->hoverZone = ZONE_NONE; }
    else if (e->hover > idx) e->hover--;
}

bool evictOldestApp(HudEngine* e) {
    const int i = evictIndex(e->panels);
    if (i < 0) return false;
    Panel& p = e->panels[i];
    if (e->backend && p.displayId >= 0)
        e->backend->removeTasksOnDisplay(p.displayId);
    closePanel(e, i);
    return true;
}

void updatePanels(HudEngine* e) {
    if (!e->backend) return;
    for (auto& p : e->panels) e->backend->updatePanel(&p);
}

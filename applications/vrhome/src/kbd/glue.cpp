#include "kbd.h"

#include "../hud/engine.h"
#include "../common/config.h"
#include "../common/log.h"

#include <GLES2/gl2.h>
#include <cstring>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

static void kbdEnsure(HudEngine* e) {
    if (e->kbd.st) return;
    GLuint tex = 0;
    const GLenum target = e->backend->externalPanelTextures()
        ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
    glGenTextures(1, &tex);
    glBindTexture(target, tex);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    e->kbd.externalTex = e->backend->externalPanelTextures();
    if (!e->backend->createKbdSurface(&e->kbd, tex, kKbdW, kKbdH,
                                      kKbdDpi)) {
        glDeleteTextures(1, &tex);
        return;
    }
    e->kbd.tex = tex;
    memset(e->kbd.stMat, 0, sizeof(e->kbd.stMat));
    LOGI("kbd surface ready");
}

void kbdTick(HudEngine* e) {
    if (!e->backend) return;
    kbdEnsure(e);
    if (!e->kbd.st) return;
    if (e->backend->takeKbdQuery()) e->backend->sendKbdSurface();
    const KbdState s = e->backend->kbdState();
    e->kbd.shown = s.shown;
    e->kbd.displayId = s.displayId;
    e->kbd.only = s.only;
    if (!e->kbd.shown) {
        if (e->kbd.hover) { e->kbd.hover = false; e->kbd.pressed = false; }
        return;
    }
    glBindTexture(e->kbd.externalTex ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D,
                  e->kbd.tex);
    e->backend->updateKbd(&e->kbd);
}

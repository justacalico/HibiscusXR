#include "notif.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../dock/dock.h"
#include "../render/shape.h"
#include "../text/draw.h"
#include "../text/glyphs.h"

#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>

static std::string jstr(JNIEnv* env, jstring s) {
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    env->DeleteLocalRef(s);
    return out;
}

void syncNotifs(HudEngine* e) {
    if (!e->bridge || !e->mNotifVer || !e->notifCls) return;
    JNIEnv* env = threadEnv(e->vm);
    const int v = env->CallIntMethod(e->bridge, e->mNotifVer);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (v == e->notifVer) return;
    e->notifVer = v;
    std::vector<NotifItem> raw;
    jobjectArray arr =
        (jobjectArray)env->CallObjectMethod(e->bridge, e->mNotifs);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (arr) {
        const int n = env->GetArrayLength(arr);
        for (int i = 0; i < n; ++i) {
            jobject o = env->GetObjectArrayElement(arr, i);
            if (!o) continue;
            NotifItem it;
            it.key   = jstr(env, (jstring)env->GetObjectField(o, e->fNotifKey));
            it.pkg   = jstr(env, (jstring)env->GetObjectField(o, e->fNotifPkg));
            it.title = jstr(env, (jstring)env->GetObjectField(o, e->fNotifTitle));
            it.text  = jstr(env, (jstring)env->GetObjectField(o, e->fNotifText));
            it.postMs = env->GetLongField(o, e->fNotifMs);
            it.clearable = env->GetBooleanField(o, e->fNotifClear) == JNI_TRUE;
            if (!it.key.empty()) raw.push_back(it);
            env->DeleteLocalRef(o);
        }
    }
    e->notifsAll = buildNotifs(raw);
    // icons resolve through the same cache the dock uses
    for (auto& n : e->notifsAll) iconFor(e, n.pkg);
}

void notifDismiss(HudEngine* e, int idx) {
    if (!e->bridge || !e->mDismissNotif) return;
    if (idx < 0 || idx >= (int)e->notifs.size()) return;
    const NotifItem& n = e->notifs[idx];
    if (!n.clearable) return;
    JNIEnv* env = threadEnv(e->vm);
    jstring jk = env->NewStringUTF(n.key.c_str());
    env->CallVoidMethod(e->bridge, e->mDismissNotif, jk);
    env->DeleteLocalRef(jk);
    if (env->ExceptionCheck()) env->ExceptionClear();
    LOGI("notif dismiss %s", n.pkg.c_str());
}

// one text line on the card plane, clipped to the text column
static void cardText(HudEngine* e, const Mat4& vp, const char* utf8,
                     const float o[3], const float r[3], const float up[3],
                     float mPerPx, float maxW, float cr, float cg, float cb) {
    if (!*utf8 || !e->font.ok) return;
    const std::string line = clipText(e->font.set, utf8, mPerPx, maxW);
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                       1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"), cr, cg, cb);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    drawTextPanel(e, line.c_str(), o, r, up, mPerPx, 0.0f);
    glUseProgram(e->shapeProg);
}

void drawNotifStack(HudEngine* e, const Mat4& vp, float yaw, float pitch,
                    float lift) {
    const int n = (int)e->notifs.size();
    if (n <= 0) return;
    float c[3], r[3], up[3];
    notifCenter(yaw, pitch, lift, e->ringPos, c, r, up);
    const float hw = kNotifCardW * 0.5f;
    const float hh = kNotifCardH * 0.5f;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    for (int i = 0; i < n; ++i) {
        const NotifItem& it = e->notifs[i];
        const float yc = notifCardY(i, n);
        const float cc[3] = {c[0] + up[0] * yc, c[1] + up[1] * yc,
                             c[2] + up[2] * yc};
        const bool hov = e->notifHover == i;

        // drop shadow behind the card: box is the card silhouette so only
        // the falloff reaches past it
        const float shc[3] = {cc[0] - up[0] * 0.012f, cc[1] - up[1] * 0.012f,
                              cc[2] - up[2] * 0.012f};
        const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.30f};
        shapeQuad(e, vp, shc, r, up, -0.012f, 0.0f, hw + 0.03f, hh + 0.03f,
                  hw, hh, 0.024f, -1.0f, 0.03f, shCol);
        // card body; a hovered card brightens a touch
        const float body[4] = {0.09f, 0.10f, 0.14f, hov ? 0.95f : 0.88f};
        shapeQuad(e, vp, cc, r, up, 0.004f, 0.0f, hw, hh, hw, hh,
                  0.022f, 0.0f, 0.0025f, body);
        if (hov) {
            const float hl[4] = {1.0f, 1.0f, 1.0f, 0.08f};
            shapeQuad(e, vp, cc, r, up, 0.006f, 0.0f, hw, hh, hw, hh,
                      0.022f, 0.0f, 0.002f, hl);
        }

        // app icon on the left; the dock's letter-tile fallback for pkgs
        // without one
        const float ix = -hw + kNotifPad + kNotifIconHW;
        const float ic[3] = {cc[0] + r[0] * ix, cc[1] + r[1] * ix,
                             cc[2] + r[2] * ix};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;
        if (icon && icon->tex) {
            glUseProgram(e->iconProg);
            const GLint uMVP = glGetUniformLocation(e->iconProg, "uMVP");
            const GLint uTex = glGetUniformLocation(e->iconProg, "uTex");
            const GLint uHalf = glGetUniformLocation(e->iconProg, "uHalf");
            const GLint uRad = glGetUniformLocation(e->iconProg, "uRadius");
            const GLint uAl = glGetUniformLocation(e->iconProg, "uAlpha");
            const GLint aPos = glGetAttribLocation(e->iconProg, "aPos");
            const GLint aUV = glGetAttribLocation(e->iconProg, "aUV");
            const float s = kNotifIconHW;
            const float q[4][5] = {
                {ic[0]-r[0]*s-up[0]*s, ic[1]-r[1]*s-up[1]*s,
                 ic[2]-r[2]*s-up[2]*s, 0.0f, 1.0f},
                {ic[0]+r[0]*s-up[0]*s, ic[1]+r[1]*s-up[1]*s,
                 ic[2]+r[2]*s-up[2]*s, 1.0f, 1.0f},
                {ic[0]+r[0]*s+up[0]*s, ic[1]+r[1]*s+up[1]*s,
                 ic[2]+r[2]*s+up[2]*s, 1.0f, 0.0f},
                {ic[0]-r[0]*s+up[0]*s, ic[1]-r[1]*s+up[1]*s,
                 ic[2]-r[2]*s+up[2]*s, 0.0f, 0.0f},
            };
            const int tris[6] = {0,1,2, 0,2,3};
            float verts[30];
            for (int t = 0; t < 6; ++t)
                memcpy(verts + t*5, q[tris[t]], 20);
            glUniformMatrix4fv(uMVP, 1, GL_FALSE, vp.m);
            glUniform2f(uHalf, s, s);
            glUniform1f(uRad, s * kIconRad);
            glUniform1f(uAl, 1.0f);
            glUniform1i(uTex, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, icon->tex);
            glBindBuffer(GL_ARRAY_BUFFER, e->panelVbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts,
                         GL_STREAM_DRAW);
            glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 20,
                                  (void*)0);
            glVertexAttribPointer(aUV, 2, GL_FLOAT, GL_FALSE, 20,
                                  (void*)12);
            glEnableVertexAttribArray(aPos);
            glEnableVertexAttribArray(aUV);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(aPos);
            glDisableVertexAttribArray(aUV);
            glUseProgram(e->shapeProg);
        } else {
            // no icon: a letter tile derived from the app label
            const char* lb = icon && !icon->label.empty()
                             ? icon->label.c_str() : it.pkg.c_str();
            const float pc[4] = {0.24f, 0.30f, 0.44f, 1.0f};
            shapeQuad(e, vp, ic, r, up, 0.008f, 0.0f, kNotifIconHW,
                      kNotifIconHW, kNotifIconHW, kNotifIconHW,
                      kNotifIconHW * kIconRad, 0.0f, 0.002f, pc);
            if (*lb && e->font.ok) {
                char ch[2] = {*lb, 0};
                const float ts = kNotifIconHW * 1.1f;
                const float tw = measureText(e, ch, ts) * 0.5f;
                float gt, gb, yo = 0.0f;
                if (textBounds(e->font.set, ch, ts, &gt, &gb))
                    yo = -(gt + gb) * 0.5f;
                float o[3] = {ic[0] - r[0]*tw + up[0]*yo - ic[0]*0.010f,
                              ic[1] - r[1]*tw + up[1]*yo - ic[1]*0.010f,
                              ic[2] - r[2]*tw + up[2]*yo - ic[2]*0.010f};
                glUseProgram(e->textProg);
                glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                                   1, GL_FALSE, vp.m);
                glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                            1.0f, 1.0f, 1.0f);
                glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, e->font.tex);
                drawTextPanel(e, ch, o, r, up, ts, 0.0f);
                glUseProgram(e->shapeProg);
            }
        }

        // title + body in the text column; title falls back to the app
        // label so a bare notification still says who posted it
        const float tx = ix + kNotifIconHW + 0.014f;
        const float textMax = hw - kNotifPad - kNotifBadgeR * 2.0f - tx;
        const char* title = !it.title.empty() ? it.title.c_str()
            : (icon && !icon->label.empty() ? icon->label.c_str()
                                            : it.pkg.c_str());
        float to[3] = {cc[0] + r[0]*tx + up[0]*0.012f,
                       cc[1] + r[1]*tx + up[1]*0.012f,
                       cc[2] + r[2]*tx + up[2]*0.012f};
        to[0] -= cc[0] * 0.010f; to[1] -= cc[1] * 0.010f;
        to[2] -= cc[2] * 0.010f;
        cardText(e, vp, title, to, r, up, 0.0018f, textMax,
                 1.0f, 1.0f, 1.0f);
        float bo[3] = {cc[0] + r[0]*tx - up[0]*0.026f,
                       cc[1] + r[1]*tx - up[1]*0.026f,
                       cc[2] + r[2]*tx - up[2]*0.026f};
        bo[0] -= cc[0] * 0.010f; bo[1] -= cc[1] * 0.010f;
        bo[2] -= cc[2] * 0.010f;
        cardText(e, vp, it.text.c_str(), bo, r, up, 0.0014f, textMax,
                 0.75f, 0.78f, 0.85f);

        // dismiss badge on a clearable card, top-right
        if (it.clearable) {
            float bx, by;
            notifBadgeAt(&bx, &by);
            const float bc[3] = {cc[0] + r[0]*bx + up[0]*by,
                                 cc[1] + r[1]*bx + up[1]*by,
                                 cc[2] + r[2]*bx + up[2]*by};
            const bool bhov = hov && e->notifZone == NZONE_CLOSE;
            const float bcol[4] = {bhov ? 0.75f : 0.10f,
                                   bhov ? 0.22f : 0.10f,
                                   bhov ? 0.20f : 0.12f, 0.92f};
            shapeQuad(e, vp, bc, r, up, 0.011f, 0.0f, kNotifBadgeR,
                      kNotifBadgeR, kNotifBadgeR, kNotifBadgeR,
                      kNotifBadgeR, 0.0f, 0.0015f, bcol);
            const float xcol[4] = {1.0f, 1.0f, 1.0f, 0.95f};
            const float il = kNotifBadgeR * 0.52f, itw = 0.0020f;
            shapeQuad(e, vp, bc, r, up, 0.012f, 0.785398f, il, itw,
                      il, itw, itw, 0.0f, 0.001f, xcol);
            shapeQuad(e, vp, bc, r, up, 0.012f, -0.785398f, il, itw,
                      il, itw, itw, 0.0f, 0.001f, xcol);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

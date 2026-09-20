#include "dock.h"

#include "layout.h"
#include "../hud/engine.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/jni.h"
#include "../common/log.h"
#include "../render/shape.h"
#include "../text/draw.h"

#include <android/bitmap.h>
#include <GLES2/gl2.h>

#include <cmath>
#include <cstring>
#include <ctime>

static long long nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// ------------------------------------------------------------- sync

// pull the icon bitmap up into a GL texture; 0 on failure keeps the letter
// tile fallback
static unsigned loadIconTex(HudEngine* e, JNIEnv* env, const char* pkg) {
    if (!e->mAppIcon) return 0;
    jstring jpkg = env->NewStringUTF(pkg);
    jobject bmp = env->CallObjectMethod(e->bridge, e->mAppIcon, jpkg);
    env->DeleteLocalRef(jpkg);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return 0; }
    if (!bmp) return 0;
    AndroidBitmapInfo info;
    unsigned tex = 0;
    if (AndroidBitmap_getInfo(env, bmp, &info) == ANDROID_BITMAP_RESULT_SUCCESS
            && info.format == ANDROID_BITMAP_FORMAT_RGBA_8888) {
        void* px = nullptr;
        if (AndroidBitmap_lockPixels(env, bmp, &px) ==
                    ANDROID_BITMAP_RESULT_SUCCESS && px) {
            GLuint t = 0;
            glGenTextures(1, &t);
            glBindTexture(GL_TEXTURE_2D, t);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, px);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);
            AndroidBitmap_unlockPixels(env, bmp);
            tex = t;
        }
    }
    env->DeleteLocalRef(bmp);
    return tex;
}

static void pullPins(HudEngine* e, JNIEnv* env) {
    if (!e->mTakePins) return;
    jobjectArray arr =
        (jobjectArray)env->CallObjectMethod(e->bridge, e->mTakePins);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (!arr) return;
    e->dockPins.clear();
    const int n = env->GetArrayLength(arr);
    for (int i = 0; i < n; ++i) {
        jstring s = (jstring)env->GetObjectArrayElement(arr, i);
        if (!s) continue;
        const char* c = env->GetStringUTFChars(s, nullptr);
        e->dockPins.push_back(c);
        env->ReleaseStringUTFChars(s, c);
        env->DeleteLocalRef(s);
    }
}

static void pullXr(HudEngine* e, JNIEnv* env) {
    if (!e->mVrVer || !e->mRunningVr || !e->pendingCls) return;
    const int v = env->CallIntMethod(e->bridge, e->mVrVer);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (v == e->dockXrVer) return;
    e->dockXrVer = v;
    e->dockXr.clear();
    jobjectArray arr =
        (jobjectArray)env->CallObjectMethod(e->bridge, e->mRunningVr);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (!arr) return;
    const int n = env->GetArrayLength(arr);
    for (int i = 0; i < n; ++i) {
        jobject p = env->GetObjectArrayElement(arr, i);
        if (!p) continue;
        XrTask t;
        t.taskId = env->GetIntField(p, e->fPendTask);
        jstring s = (jstring)env->GetObjectField(p, e->fPendPkg);
        if (s) {
            const char* c = env->GetStringUTFChars(s, nullptr);
            t.pkg = c;
            env->ReleaseStringUTFChars(s, c);
            env->DeleteLocalRef(s);
        }
        e->dockXr.push_back(t);
        env->DeleteLocalRef(p);
    }
}

// wifi link, battery level and charging flag for the status cluster;
// a plain int array keeps the JNI shape trivial
static void pullSys(HudEngine* e, JNIEnv* env) {
    if (!e->mSysStatus) return;
    jintArray arr =
        (jintArray)env->CallObjectMethod(e->bridge, e->mSysStatus);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return; }
    if (!arr) return;
    if (env->GetArrayLength(arr) >= 3) {
        jint* v = env->GetIntArrayElements(arr, nullptr);
        if (v) {
            e->sysWifi = v[0];
            e->sysBatt = v[1];
            e->sysChg = v[2];
            env->ReleaseIntArrayElements(arr, v, JNI_ABORT);
        }
    }
    env->DeleteLocalRef(arr);
}

// icon + label + vr flag for one package, fetched once and kept in
// e->dockIcons; the notification cards share the same cache
DockIcon& iconFor(HudEngine* e, const std::string& pkg) {
    DockIcon& ic = e->dockIcons[pkg];
    JNIEnv* env = threadEnv(e->vm);
    if (!ic.tried) {
        ic.tried = true;
        ic.tex = loadIconTex(e, env, pkg.c_str());
    }
    if (ic.label.empty() && e->mAppLabel) {
        jstring jpkg = env->NewStringUTF(pkg.c_str());
        jstring jl = (jstring)env->CallObjectMethod(e->bridge,
                        e->mAppLabel, jpkg);
        env->DeleteLocalRef(jpkg);
        if (env->ExceptionCheck()) env->ExceptionClear();
        else if (jl) {
            const char* c = env->GetStringUTFChars(jl, nullptr);
            ic.label = c;
            env->ReleaseStringUTFChars(jl, c);
            env->DeleteLocalRef(jl);
        }
    }
    if (!ic.vrTried && e->mIsVr) {
        ic.vrTried = true;
        jstring jpkg = env->NewStringUTF(pkg.c_str());
        ic.vr = env->CallBooleanMethod(e->bridge, e->mIsVr, jpkg)
                == JNI_TRUE;
        env->DeleteLocalRef(jpkg);
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    return ic;
}

void syncDock(HudEngine* e) {
    JNIEnv* env = threadEnv(e->vm);
    if (e->bridge) {
        pullPins(e, env);
        pullXr(e, env);
        pullSys(e, env);
    }
    // the clock drives the layout: its measured width is the cluster's
    // first slot
    std::time_t tt = std::time(nullptr);
    std::strftime(e->sysClock, sizeof(e->sysClock), "%H:%M",
                  std::localtime(&tt));
    e->dockSys.clockW = measureText(e, e->sysClock, kSysPx);
    e->dock = buildDock(e->dockPins, e->panels, e->dockXr);
    e->dockHW = dockLayout(e->dock, e->dockSys);
    if (!e->bridge) return;
    for (auto& it : e->dock) {
        const DockIcon& ic = iconFor(e, it.pkg);
        it.label = ic.label;
        if (ic.vr) it.vr = true;
    }
}

// ------------------------------------------------------------- actions

void dockActivate(HudEngine* e, int idx) {
    if (!e->bridge || idx < 0 || idx >= (int)e->dock.size()) return;
    const DockItem it = e->dock[idx];
    JNIEnv* env = threadEnv(e->vm);
    if (it.kind == DK_QUICK) {
        // a live quick panel gets focused like any other running item
        if (it.panelIdx >= 0 && it.panelIdx < (int)e->panels.size() &&
                e->panels[it.panelIdx].pkg == it.pkg) {
            Panel& p = e->panels[it.panelIdx];
            p.minimized = false;
            if (p.taskId >= 0) {
                env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            return;
        }
        queueLaunch(kQuickPanelPkg);
        return;
    }
    if (it.vr && it.running) {
        // an immersive app owns the display: focus its task and drop the
        // menu so the user lands inside it
        if (it.taskId >= 0) {
            env->CallVoidMethod(e->bridge, e->mFocusTask, it.taskId);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        if (e->mDismiss) {
            env->CallVoidMethod(e->bridge, e->mDismiss);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        return;
    }
    if (it.panelIdx >= 0 && it.panelIdx < (int)e->panels.size()) {
        Panel& p = e->panels[it.panelIdx];
        if (p.pkg != it.pkg) { queueLaunch(it.pkg.c_str()); return; }
        p.minimized = false;
        if (p.taskId >= 0) {
            env->CallVoidMethod(e->bridge, e->mFocusTask, p.taskId);
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        return;
    }
    queueLaunch(it.pkg.c_str());
}

void dockClose(HudEngine* e, int idx) {
    if (!e->bridge || idx < 0 || idx >= (int)e->dock.size()) return;
    const DockItem& it = e->dock[idx];
    if (it.taskId < 0) return;
    JNIEnv* env = threadEnv(e->vm);
    env->CallVoidMethod(e->bridge, e->mRemoveTask, it.taskId);
    if (env->ExceptionCheck()) env->ExceptionClear();
}

void dockPushPins(HudEngine* e) {
    if (!e->bridge || !e->mSetPins) return;
    JNIEnv* env = threadEnv(e->vm);
    jclass scls = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray((int)e->dockPins.size(),
                                           scls, nullptr);
    for (int i = 0; i < (int)e->dockPins.size(); ++i)
        env->SetObjectArrayElement(arr, i,
                env->NewStringUTF(e->dockPins[i].c_str()));
    env->CallVoidMethod(e->bridge, e->mSetPins, arr);
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(arr);
    env->DeleteLocalRef(scls);
}

void dockTogglePin(HudEngine* e, const char* pkg) {
    e->dockPins = pinToggle(e->dockPins, pkg);
    LOGI("dock pin toggle %s -> %zu pins", pkg, e->dockPins.size());
    dockPushPins(e);
}

void dockTick(HudEngine* e) {
    if (e->dockPress < 0) { e->dockPinP = 0.0f; return; }
    const DockItem* it = nullptr;
    if (e->dockPress < (int)e->dock.size() &&
            e->dock[e->dockPress].pkg == e->dockPressPkg)
        it = &e->dock[e->dockPress];
    if (!it || !dockPinnable(*it)) { e->dockPinP = 0.0f; return; }
    const float p = (float)(nowMs() - e->dockPressMs) / (float)kDockPinMs;
    e->dockPinP = p < 0.0f ? 0.0f : p > 1.0f ? 1.0f : p;
    if (p < 1.0f || e->dockPinDone) return;
    e->dockPinDone = true;
    dockTogglePin(e, it->pkg.c_str());
}

// ------------------------------------------------------------- draw

// fallback tile for an app with no icon: a coloured rounded square carrying
// the label's first letter
static void drawLetterTile(HudEngine* e, const Mat4& vp, const float ic[3],
                           const float r[3], const float up[3], float s,
                           const char* label) {
    static const float pal[][3] = {
        {0.30f, 0.36f, 0.52f}, {0.36f, 0.30f, 0.50f}, {0.22f, 0.42f, 0.48f},
        {0.40f, 0.30f, 0.34f}, {0.26f, 0.44f, 0.36f}, {0.44f, 0.38f, 0.26f},
    };
    unsigned h = 0;
    for (const char* q = label; *q; ++q) h = h * 31 + (unsigned char)*q;
    const float* pc = pal[h % 6];
    const float col[4] = {pc[0], pc[1], pc[2], 1.0f};
    shapeQuad(e, vp, ic, r, up, 0.008f, 0.0f, s, s, s, s, s * kIconRad,
              0.0f, 0.002f, col);
    if (*label && e->font.ok) {
        char ch[2] = {*label, 0};
        // glyph height lands a bit under the tile's: mPerPx is metres per
        // font pixel, not a fraction of the tile
        const float ts = s * 0.04f;
        const float tw = measureText(e, ch, ts) * 0.5f;
        float gt, gb;
        float yo = 0.0f;
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

void drawDock(HudEngine* e, const Mat4& vp) {
    if (e->dock.empty() || e->dockHW <= 0.0f) return;
    float c[3], r[3], up[3];
    dockCenter(e->dockYaw, e->dockPitch, e->ringPos, c, r, up);
    const float hw = e->dockHW, hh = kDockBarH * 0.5f;
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUseProgram(e->shapeProg);
    // shadow under the strip: the box is the bar's own silhouette so only
    // the soft falloff reaches past it - a padded box reads as a dark slab
    const float shc[3] = {c[0] - up[0] * 0.02f, c[1] - up[1] * 0.02f,
                          c[2] - up[2] * 0.02f};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.30f};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.05f, hh + 0.05f,
              hw, hh, hh, -1.0f, 0.05f, shCol);
    // the bar itself
    const float barCol[4] = {0.07f, 0.08f, 0.11f, 0.82f};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, hh, 0.0f,
              0.003f, barCol);
    // group separators
    const float sepCol[4] = {1.0f, 1.0f, 1.0f, 0.22f};
    for (auto& it : e->dock) {
        if (!it.sep) continue;
        const float sx = it.x - kDockIconHW - (kDockGap + kDockSepW) * 0.5f;
        const float sc[3] = {c[0] + r[0] * sx, c[1] + r[1] * sx,
                             c[2] + r[2] * sx};
        shapeQuad(e, vp, sc, r, up, 0.006f, 0.0f, 0.0012f, hh * 0.55f,
                  0.0012f, hh * 0.55f, 0.0012f, 0.0f, 0.0015f, sepCol);
    }

    // status cluster on the left: clock, wifi fan, battery and bell, then
    // the separator that splits them off the app icons. Positions came out
    // of dockLayout, state out of the last bridge pull
    const DockStatus& st = e->dockSys;
    if (!e->dock.empty()) {
        const float sc[3] = {c[0] + r[0] * st.sepX, c[1] + r[1] * st.sepX,
                             c[2] + r[2] * st.sepX};
        shapeQuad(e, vp, sc, r, up, 0.006f, 0.0f, 0.0012f, hh * 0.55f,
                  0.0012f, hh * 0.55f, 0.0012f, 0.0f, 0.0015f, sepCol);
    }

    // clock, vertically centred on the bar like the window labels
    if (e->font.ok) {
        float gt, gb, yo = 0.0f;
        if (textBounds(e->font.set, e->sysClock, kSysPx, &gt, &gb))
            yo = -(gt + gb) * 0.5f;
        float co[3] = {c[0] + r[0] * st.clockX + up[0] * yo,
                       c[1] + r[1] * st.clockX + up[1] * yo,
                       c[2] + r[2] * st.clockX + up[2] * yo};
        co[0] -= c[0] * 0.010f; co[1] -= c[1] * 0.010f; co[2] -= c[2] * 0.010f;
        glUseProgram(e->textProg);
        glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                           1, GL_FALSE, vp.m);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, e->font.tex);
        drawTextPanel(e, e->sysClock, co, r, up, kSysPx, 0.0f);
        glUseProgram(e->shapeProg);
    }

    // wifi fan: the apex dot sits low in the slot, three arcs open upward;
    // bright while the link is up, dim when it drops
    {
        const float wcol[4] = {1.0f, 1.0f, 1.0f,
                               e->sysWifi ? 0.92f : 0.25f};
        const float wy = -0.026f;
        const float wc[3] = {c[0] + r[0]*st.wifiX + up[0]*wy,
                             c[1] + r[1]*st.wifiX + up[1]*wy,
                             c[2] + r[2]*st.wifiX + up[2]*wy};
        const float wr[3] = {0.014f, 0.027f, 0.040f};
        for (int a = 0; a < 3; ++a)
            shapeQuad(e, vp, wc, r, up, 0.008f, 0.0f,
                      wr[a] + 0.008f, wr[a] + 0.008f, wr[a], wr[a],
                      wr[a], 0.0045f, 0.0025f, wcol, -1.0f, 1.15f);
        shapeQuad(e, vp, wc, r, up, 0.008f, 0.0f, 0.006f, 0.006f,
                  0.0045f, 0.0045f, 0.0045f, 0.0f, 0.0015f, wcol);
    }

    // battery: outline body, level fill inside, tip nub on the right;
    // amber while charging, red under a fifth, white otherwise
    {
        const float bw = 0.044f, bh = 0.024f, bt = 0.0030f;
        const float bc[3] = {c[0] + r[0]*st.battX, c[1] + r[1]*st.battX,
                             c[2] + r[2]*st.battX};
        const float ocol[4] = {1.0f, 1.0f, 1.0f, 0.80f};
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.0f, bw * 0.5f + 0.006f,
                  bh * 0.5f + 0.006f, bw * 0.5f, bh * 0.5f, bh * 0.30f,
                  bt, 0.002f, ocol);
        float lvl = e->sysBatt / 100.0f;
        if (lvl < 0.0f) lvl = 0.0f; else if (lvl > 1.0f) lvl = 1.0f;
        const float fw = (bw - bt * 4.0f) * lvl, fhh = bh * 0.5f - bt * 2.0f;
        // the fill hugs the body's left inner edge
        const float fx = -(bw * 0.5f) + bt * 2.0f + fw * 0.5f;
        const float fc[3] = {bc[0] + r[0] * fx, bc[1] + r[1] * fx,
                             bc[2] + r[2] * fx};
        const float fcol[4] = {
            e->sysChg ? 1.0f : lvl < 0.20f ? 0.95f : 1.0f,
            e->sysChg ? 0.62f : lvl < 0.20f ? 0.30f : 1.0f,
            e->sysChg ? 0.15f : lvl < 0.20f ? 0.25f : 1.0f, 0.85f};
        if (fw > 0.001f)
            shapeQuad(e, vp, fc, r, up, 0.008f, 0.0f, fw * 0.5f, fhh,
                      fw * 0.5f, fhh, fhh * 0.3f, 0.0f, 0.0015f, fcol);
        const float tc[3] = {c[0] + r[0]*(st.battX + bw * 0.5f + 0.004f),
                             c[1] + r[1]*(st.battX + bw * 0.5f + 0.004f),
                             c[2] + r[2]*(st.battX + bw * 0.5f + 0.004f)};
        shapeQuad(e, vp, tc, r, up, 0.008f, 0.0f, 0.0025f, bh * 0.18f,
                  0.0025f, bh * 0.18f, 0.002f, 0.0f, 0.0012f, ocol);
    }

    // bell: rounded-top body over a lip with a clapper dot; dim while the
    // shade is empty, a red shoulder dot while notifications wait
    {
        const bool any = !e->notifsAll.empty();
        const float bcol[4] = {1.0f, 1.0f, 1.0f, any ? 0.90f : 0.30f};
        const float bx = st.bellX;
        const float bc[3] = {c[0] + r[0]*bx + up[0]*0.002f,
                             c[1] + r[1]*bx + up[1]*0.002f,
                             c[2] + r[2]*bx + up[2]*0.002f};
        shapeQuad(e, vp, bc, r, up, 0.008f, 0.0f, 0.015f, 0.017f,
                  0.013f, 0.015f, 0.012f, 0.0f, 0.0018f, bcol, 0.002f);
        const float lc[3] = {c[0] + r[0]*bx - up[0]*0.014f,
                             c[1] + r[1]*bx - up[1]*0.014f,
                             c[2] + r[2]*bx - up[2]*0.014f};
        shapeQuad(e, vp, lc, r, up, 0.008f, 0.0f, 0.021f, 0.004f,
                  0.019f, 0.004f, 0.003f, 0.0f, 0.0015f, bcol);
        const float kc[3] = {c[0] + r[0]*bx - up[0]*0.023f,
                             c[1] + r[1]*bx - up[1]*0.023f,
                             c[2] + r[2]*bx - up[2]*0.023f};
        shapeQuad(e, vp, kc, r, up, 0.008f, 0.0f, 0.005f, 0.005f,
                  0.004f, 0.004f, 0.004f, 0.0f, 0.0015f, bcol);
        if (any) {
            const float dc[3] = {c[0] + r[0]*(bx + 0.014f) + up[0]*0.018f,
                                 c[1] + r[1]*(bx + 0.014f) + up[1]*0.018f,
                                 c[2] + r[2]*(bx + 0.014f) + up[2]*0.018f};
            const float dcol[4] = {0.95f, 0.25f, 0.20f, 0.95f};
            shapeQuad(e, vp, dc, r, up, 0.009f, 0.0f, 0.007f, 0.007f,
                      0.006f, 0.006f, 0.006f, 0.0f, 0.0015f, dcol);
        }
    }

    for (int i = 0; i < (int)e->dock.size(); ++i) {
        const DockItem& it = e->dock[i];
        const bool hov = e->dockHover == i;
        const float s = kDockIconHW * (hov ? 1.14f : 1.0f);
        const float ic[3] = {c[0] + r[0]*it.x + up[0]*kDockIconY,
                             c[1] + r[1]*it.x + up[1]*kDockIconY,
                             c[2] + r[2]*it.x + up[2]*kDockIconY};
        const DockIcon* icon = nullptr;
        auto f = e->dockIcons.find(it.pkg);
        if (f != e->dockIcons.end()) icon = &f->second;

        if (hov) {
            const float hl[4] = {1.0f, 1.0f, 1.0f, 0.10f};
            shapeQuad(e, vp, ic, r, up, 0.006f, 0.0f, s + 0.018f,
                      s + 0.018f, s + 0.018f, s + 0.018f,
                      (s + 0.018f) * kIconRad, 0.0f, 0.002f, hl);
        }

        const float alpha = it.minimized ? 0.45f : 1.0f;
        if (icon && icon->tex) {
            glUseProgram(e->iconProg);
            const GLint uMVP = glGetUniformLocation(e->iconProg, "uMVP");
            const GLint uTex = glGetUniformLocation(e->iconProg, "uTex");
            const GLint uHalf = glGetUniformLocation(e->iconProg, "uHalf");
            const GLint uRad = glGetUniformLocation(e->iconProg, "uRadius");
            const GLint uAl = glGetUniformLocation(e->iconProg, "uAlpha");
            const GLint aPos = glGetAttribLocation(e->iconProg, "aPos");
            const GLint aUV = glGetAttribLocation(e->iconProg, "aUV");
            // bitmaps upload top-row-first, so v=0 is the image's top
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
            glUniform1f(uAl, alpha);
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
            glUseProgram(e->shapeProg);
            drawLetterTile(e, vp, ic, r, up, s,
                           it.label.empty() ? it.pkg.c_str()
                                            : it.label.c_str());
            glUseProgram(e->shapeProg);
        }
        if (it.minimized) {
            const float dim[4] = {0.0f, 0.0f, 0.0f, 0.35f};
            shapeQuad(e, vp, ic, r, up, 0.010f, 0.0f, s, s, s, s,
                      s * kIconRad, 0.0f, 0.002f, dim);
        }

        // immersive marker: an amber ring around live/pinned XR items
        if (it.vr) {
            const float vc[4] = {1.0f, 0.62f, 0.15f, hov ? 0.95f : 0.65f};
            shapeQuad(e, vp, ic, r, up, 0.009f, 0.0f, s + 0.006f,
                      s + 0.006f, s + 0.006f, s + 0.006f, s + 0.006f,
                      0.0018f, 0.0015f, vc);
        }

        // running dot under the icon
        if (it.running) {
            const float dc[3] = {ic[0] - up[0] * (kDockIconHW + 0.026f),
                                 ic[1] - up[1] * (kDockIconHW + 0.026f),
                                 ic[2] - up[2] * (kDockIconHW + 0.026f)};
            const float dcol[4] = {it.vr ? 1.0f : 1.0f,
                                   it.vr ? 0.62f : 1.0f,
                                   it.vr ? 0.15f : 1.0f,
                                   it.minimized ? 0.4f : 0.85f};
            shapeQuad(e, vp, dc, r, up, 0.008f, 0.0f, 0.007f, 0.007f,
                      0.007f, 0.007f, 0.007f, 0.0f, 0.0015f, dcol);
        }

        // close badge on a live immersive item: a disc off the icon's
        // top-right corner carrying a rotated capsule x
        if (it.vr && it.running) {
            const float bo = kDockIconHW * 0.72f;
            const float bc[3] = {c[0] + r[0]*(it.x + bo) + up[0]*(kDockIconY + bo),
                                 c[1] + r[1]*(it.x + bo) + up[1]*(kDockIconY + bo),
                                 c[2] + r[2]*(it.x + bo) + up[2]*(kDockIconY + bo)};
            const bool bhov = hov && e->dockZone == DZONE_CLOSE;
            const float bcol[4] = {bhov ? 0.75f : 0.10f,
                                   bhov ? 0.22f : 0.10f,
                                   bhov ? 0.20f : 0.12f, 0.92f};
            shapeQuad(e, vp, bc, r, up, 0.011f, 0.0f, kDockBadgeR,
                      kDockBadgeR, kDockBadgeR, kDockBadgeR, kDockBadgeR,
                      0.0f, 0.0015f, bcol);
            const float xcol[4] = {1.0f, 1.0f, 1.0f, 0.95f};
            const float il = kDockBadgeR * 0.52f, it2 = 0.0024f;
            shapeQuad(e, vp, bc, r, up, 0.012f, 0.785398f, il, it2,
                      il, it2, it2, 0.0f, 0.001f, xcol);
            shapeQuad(e, vp, bc, r, up, 0.012f, -0.785398f, il, it2,
                      il, it2, it2, 0.0f, 0.001f, xcol);
        }

        // pin-hold fill: a white ring tightening around the icon
        if (e->dockPress == i && e->dockPinP > 0.0f && dockPinnable(it)) {
            const float pr[4] = {1.0f, 1.0f, 1.0f,
                                 0.25f + 0.75f * e->dockPinP};
            shapeQuad(e, vp, ic, r, up, 0.013f, 0.0f, s + 0.014f,
                      s + 0.014f, s + 0.014f, s + 0.014f, s + 0.014f,
                      0.0022f + 0.002f * e->dockPinP, 0.0015f, pr);
        }

        // label above the strip while the item is hovered
        if (hov && !it.label.empty() && e->font.ok) {
            const float ts = 0.0016f;
            const float tw = measureText(e, it.label.c_str(), ts) * 0.5f;
            float o[3] = {c[0] + r[0]*(it.x - tw) + up[0]*(hh + 0.05f),
                          c[1] + r[1]*(it.x - tw) + up[1]*(hh + 0.05f),
                          c[2] + r[2]*(it.x - tw) + up[2]*(hh + 0.05f)};
            o[0] -= c[0] * 0.010f;
            o[1] -= c[1] * 0.010f;
            o[2] -= c[2] * 0.010f;
            glUseProgram(e->textProg);
            glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                               1, GL_FALSE, vp.m);
            glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                        1.0f, 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, e->font.tex);
            drawTextPanel(e, it.label.c_str(), o, r, up, ts, 0.0f);
            glUseProgram(e->shapeProg);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

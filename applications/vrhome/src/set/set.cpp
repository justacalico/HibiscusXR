#include "set.h"

#include "icons.h"
#include "item.h"
#include "layout.h"
#include "../anim/anim.h"
#include "../bridge/bridge.h"
#include "../common/config.h"
#include "../common/palette.h"
#include "../dock/layout.h"
#include "../hud/engine.h"
#include "../panels/layout.h"
#include "../render/shape.h"
#include "../text/draw.h"

#include <GLES2/gl2.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>

void setOpen(HudEngine* e, int section) {
    if (section < 0 || section >= SEC_COUNT) section = SEC_WIFI;
    e->set.section = section;
    e->set.shown = true;
    e->set.dlg = DLG_NONE;
    e->set.scroll = 0.0f;
    e->set.rowsDirty = true;
    e->set.snapVer = -1;
    e->grid.shown = false;
    e->quick.shown = false;
    if (e->backend) e->backend->settingsSection(section);
}

void setClose(HudEngine* e) {
    e->set.shown = false;
    e->set.dlg = DLG_NONE;
    e->set.scrollHeld = false;
    e->set.slideHeld = -1;
    if (e->backend) e->backend->settingsClose();
}

void syncSet(HudEngine* e) {
    if (!e->backend) return;
    if (!e->set.shown && e->set.openT <= 0.0f) return;
    const int v = e->backend->settingsVersion();
    if (v == e->set.snapVer && !e->set.rowsDirty) return;
    e->backend->settingsStatus(&e->set.snap);
    e->set.snapVer = v;
    setBuildRows(e->set.section, e->set.snap, &e->set.rows);
    e->set.rowsDirty = false;
}

// ---------------------------------------------------------- activation

// the value text/info rows render
static const char* infoText(HudEngine* e, int txt);

static void wifiJoin(HudEngine* e) {
    SetOverlay& st = e->set;
    if (st.dlgArg < 0 || st.dlgArg >= (int)st.snap.wifi.size()) return;
    const SetWifi& n = st.snap.wifi[st.dlgArg];
    if (e->backend)
        e->backend->wifiJoin(n.ssid, n.sec, st.edit);
    st.dlg = DLG_NONE;
    st.edit.clear();
}

// one key tap in the dialog keyboard
static void setKeyTap(HudEngine* e, int key) {
    SetOverlay& st = e->set;
    const int code = setKeyCode(st.kbdPage, key, st.kbdShift);
    switch (code) {
    case SK_SHIFT:
        st.kbdShift = !st.kbdShift;
        return;
    case SK_BKSP:
        if (!st.edit.empty()) st.edit.pop_back();
        return;
    case SK_PAGE:
        st.kbdPage ^= 1;
        return;
    case SK_DONE:
        wifiJoin(e);
        return;
    default:
        if (code > 0 && st.edit.size() < 63) st.edit += (char)code;
        if (st.kbdShift) st.kbdShift = false;
        return;
    }
}

// dialog button i fires; returns true when the dialog closed
static bool dlgBtn(HudEngine* e, int btn) {
    SetOverlay& st = e->set;
    switch (st.dlg) {
    case DLG_WIFI: {
        if (st.dlgArg < 0 || st.dlgArg >= (int)st.snap.wifi.size()) {
            st.dlg = DLG_NONE;
            return true;
        }
        const SetWifi& n = st.snap.wifi[st.dlgArg];
        const int nb = setDlgBtnCount(DLG_WIFI, st);
        // left slot is Forget on saved nets; then Cancel, then Connect
        if (nb == 3 && btn == 0) {
            if (e->backend) e->backend->wifiForget(n.saved);
            st.dlg = DLG_NONE;
            return true;
        }
        if (btn == nb - 2) { st.dlg = DLG_NONE; st.edit.clear(); return true; }
        if (btn == nb - 1 && !n.conn) { wifiJoin(e); return true; }
        return false;
    }
    case DLG_BT:
        if (btn == 1 && st.dlgArg >= 0 &&
            st.dlgArg < (int)st.snap.bt.size() && e->backend)
            e->backend->btUnpair(st.snap.bt[st.dlgArg].addr);
        st.dlg = DLG_NONE;
        return true;
    case DLG_ENV:
        if (btn == 1 && st.dlgArg >= 2 &&
            st.dlgArg - 2 < (int)st.snap.envs.size() && e->backend)
            e->backend->envDelete(st.snap.envs[st.dlgArg - 2].id);
        st.dlg = DLG_NONE;
        return true;
    case DLG_REBOOT:
        if (btn == 1 && e->backend) {
            e->backend->settingsToggle(TG_DOF6, st.dlgAux != 0);
            e->backend->settingsReboot();
        }
        st.dlg = DLG_NONE;
        return true;
    default:
        st.dlg = DLG_NONE;
        return true;
    }
}

// a content-row release resolves to its effect
static void rowTap(HudEngine* e, int i, int zone, int aux) {
    SetOverlay& st = e->set;
    if (i < 0 || i >= (int)st.rows.size()) return;
    const SetRow& r = st.rows[i];
    if (!setImplemented(r.aux)) return;
    switch (r.kind) {
    case RK_TOGGLE:
        st.snap.toggles ^= (1u << r.id);
        if (e->backend)
            e->backend->settingsToggle(r.id,
                                       (st.snap.toggles >> r.id) & 1u);
        return;
    case RK_ACTION:
        if (r.id == ACT_TIMEZONE) {
            queueLaunch("com.android.settings");
            setClose(e);
        } else if (r.id == ACT_CTRL_UNBIND && e->backend) {
            e->backend->settingsAction(ACT_CTRL_UNBIND);
        }
        return;
    case RK_SEG: {
        if (aux < 0 || aux >= setSegCount(r.id)) return;
        if (r.id == PK_DOF) {
            const int on = aux;
            if (on == ((st.snap.toggles >> TG_DOF6) & 1u)) return;
            st.dlg = DLG_REBOOT;
            st.dlgAux = on;
            return;
        }
        const char* v = setSegValue(r.id, aux);
        if (!*v) return;
        const int txt = r.id == PK_THEME ? TX_THEME : TX_LANG;
        st.snap.txt[txt] = v;
        if (e->backend) e->backend->settingsText(txt, v);
        return;
    }
    case RK_SCANHDR:
        if (r.id == SEC_CTRL) {
            st.snap.flags ^= SFL_CTRL_SCAN;
            if (e->backend) e->backend->settingsToggleScan();
            return;
        }
        if (e->backend) e->backend->settingsScan(r.id);
        return;
    case RK_NET:
        if (r.aux >= 0 && r.aux < (int)st.snap.wifi.size()) {
            st.dlg = DLG_WIFI;
            st.dlgArg = r.aux;
            st.edit.clear();
            st.kbdPage = 0;
            st.kbdShift = false;
        }
        return;
    case RK_BTROW:
        if (r.aux >= 0 && r.aux < (int)st.snap.bt.size() && e->backend) {
            const SetBtDev& d = st.snap.bt[r.aux];
            if (d.bonded) {
                st.dlg = DLG_BT;
                st.dlgArg = r.aux;
            } else {
                e->backend->btPair(d.addr);
            }
        }
        return;
    case RK_IME:
        if (r.aux >= 0 && r.aux < (int)st.snap.imes.size() && e->backend)
            e->backend->imePick(st.snap.imes[r.aux].id);
        return;
    case RK_ENV: {
        const int idx = r.aux - 2;
        if (idx >= 0 && idx < (int)st.snap.envs.size() &&
            st.snap.envs[idx].st != 0) return;   // broken zip: inert
        const char* id = setEnvId(st.snap, r.aux);
        if (*id) {
            st.snap.txt[TX_ENV] = id;
            if (e->backend) e->backend->settingsText(TX_ENV, id);
        }
        return;
    }
    default:
        return;
    }
}

void setPress(HudEngine* e) {
    SetOverlay& st = e->set;
    st.press = st.hover;
    st.pressZone = st.zone;
    st.pressAux = st.aux;
    if (st.zone == SZONE_SLIDER) {
        st.slideHeld = st.hover;
        const float v = setSliderValue(st.u);
        const SetRow& r = st.rows[st.hover];
        st.snap.sld[r.id] = r.id == SL_IPD
            ? kIpdMinMm + v * (kIpdMaxMm - kIpdMinMm) : v;
        if (e->backend) {
            const float out = r.id == SL_IPD ? st.snap.sld[r.id] : v;
            e->backend->settingsSlider(r.id, out);
        }
    } else if (st.zone == SZONE_ROW || st.zone == SZONE_SEC ||
               st.zone == SZONE_BODY) {
        st.scrollHeld = st.zone == SZONE_BODY || st.zone == SZONE_ROW;
        st.grabV = st.v;
        st.grabScroll = st.scroll;
    }
}

void setRelease(HudEngine* e) {
    SetOverlay& st = e->set;
    const bool same = st.zone == st.pressZone && st.hover == st.press &&
                      st.aux == st.pressAux;
    const int pz = st.pressZone, pi = st.press, pa = st.pressAux;
    st.press = -1;
    st.pressZone = SZONE_NONE;
    st.pressAux = -1;
    st.slideHeld = -1;
    st.scrollHeld = false;
    if (!same) return;
    switch (pz) {
    case SZONE_CLOSE:
        setClose(e);
        return;
    case SZONE_SEC:
        if (pi != st.section) {
            st.section = pi;
            st.scroll = 0.0f;
            st.rowsDirty = true;
            if (e->backend) e->backend->settingsSection(pi);
        }
        return;
    case SZONE_KEY:
        setKeyTap(e, pi);
        return;
    case SZONE_BTN:
        dlgBtn(e, pi);
        return;
    case SZONE_SCAN:
        if (pi >= 0 && pi < (int)st.rows.size() && e->backend) {
            const SetRow& r = st.rows[pi];
            if (r.kind == RK_SCANHDR && r.id != SEC_IME)
                e->backend->settingsScan(r.id);
        }
        return;
    case SZONE_DEL:
        if (pi >= 0 && pi < (int)st.rows.size() &&
            st.rows[pi].kind == RK_ENV && st.rows[pi].aux >= 2) {
            st.dlg = DLG_ENV;
            st.dlgArg = st.rows[pi].aux;
        }
        return;
    case SZONE_ROW:
        rowTap(e, pi, pz, pa);
        return;
    case SZONE_SEG:
        rowTap(e, pi, pz, pa);
        return;
    default:
        return;
    }
}

void setDragTick(HudEngine* e) {
    SetOverlay& st = e->set;
    if (!e->confirmHeld || !st.shown) return;
    float c[3], r[3], up[3];
    setCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos, c, r, up);
    float u, v, t;
    if (!rayQuad(c, r, up, e->ringPos, e->aimO, e->aimD,
                 kSetHW, kSetHH, &u, &v, &t)) return;
    // a row press that slid past the slop becomes the scroll drag
    if ((st.pressZone == SZONE_ROW || st.pressZone == SZONE_SEC) &&
            !st.scrollHeld && setSlopPassed(st.grabV, v)) {
        st.pressZone = SZONE_BODY;
        st.scrollHeld = true;
        st.grabV = v;
        st.grabScroll = st.scroll;
    }
    if (st.scrollHeld)
        st.scroll = setClampScroll(st, st.grabScroll +
                                   (v - st.grabV) * kSetHH);
    if (st.slideHeld >= 0 && st.slideHeld < (int)st.rows.size()) {
        const SetRow& row = st.rows[st.slideHeld];
        if (row.kind == RK_SLIDER) {
            const float nv = setSliderValue(u);
            st.snap.sld[row.id] = row.id == SL_IPD
                ? kIpdMinMm + nv * (kIpdMaxMm - kIpdMinMm) : nv;
            if (e->backend)
                e->backend->settingsSlider(row.id, st.snap.sld[row.id]);
        }
    }
}

void setScroll(HudEngine* e, float delta) {
    if (!e->set.shown) return;
    e->set.scroll = setScrollBy(e->set, delta);
}

// ------------------------------------------------------------- draw

// uiStrings fallback: English before the first pull lands
static const char* kSetFallback[UISET_COUNT] = {
    "Settings",
    "Wi-Fi", "Bluetooth", "Controllers", "Display", "Home environment",
    "Sound", "Language and Region", "Time", "Keyboard", "Developer",
    "About",
    "Wi-Fi", "Network", "Available networks",
    "Bluetooth", "Devices",
    "Scan for controllers", "Left controller", "Right controller",
    "Unpair all",
    "Brightness", "Theme", "Eye spacing (IPD)", "Device mode",
    "Night mode",
    "Home environments", "Active environment",
    "Volume", "Microphone",
    "Language",
    "Date and time",
    "On-screen keyboard",
    "USB debugging", "Stay awake", "Show touches", "Debug HUD",
    "HibiscusXR", "Model", "Android version", "Hibiscus version",
    "Connect to a wireless network", "The network this headset is connected to",
    "Tap a network to connect or manage it", "Pair controllers and accessories",
    "Bonded and nearby Bluetooth devices",
    "Start a link scan, then hold the pairing button on each controller",
    "Link state and battery level", "Link state and battery level",
    "Forget both paired controllers",
    "Adjust the screen brightness", "Colors for the panels and the headset HUD",
    "Match the stereo eye separation to your pupils",
    "3DoF tracks head rotation only. 6DoF also tracks position",
    "Warm the display colors after dark",
    "Pick what surrounds you at home", "The environment the home shell loads",
    "Adjust the media volume", "Allow apps to use the microphone",
    "Change the system language", "Set the time zone and clock format",
    "Choose the active input method",
    "Allow a computer to debug this headset over USB",
    "Never sleep while charging", "Flash a dot where the screen is touched",
    "Keep the debug status line visible over any app",
    "The system software on this headset", "The hardware model of this headset",
    "The Android release this system runs",
    "The Hibiscus build installed on this headset",
    "Connected", "Disconnected", "Pairing…", "Paired",
    "Scanning for networks…", "Searching for devices…",
    "Scanning for controllers…",
    "Tap refresh to scan", "Tap refresh to search", "Not scanning",
    "No networks found", "No devices found", "No environments installed",
    "Secured", "Open", "Saved", "Refresh", "Pair", "Forget", "Connect",
    "Cancel", "Remove", "Reboot", "Connect to %s", "Remove %s?",
    "The zip file is deleted from the headset.",
    "Reboot required",
    "To change this setting you have to reboot the headset.",
    "Password", "Unknown", "3DoF", "6DoF", "Dark", "Light", "OLED",
    "English", "简体中文",
    "Passthrough", "Live view from the tracking cameras",
    "Built-in", "The default sky dome and floor grid",
    "Not a readable zip", "No map.obj inside", "In use",
    "Battery %d/5", "Charging", "space",
};

static const char* setStr(HudEngine* e, int i) {
    return e->uiSet[i].empty() ? kSetFallback[i] : e->uiSet[i].c_str();
}

static void sText(HudEngine* e, const Mat4& vp, const float c[3],
                  const float r[3], const float up[3],
                  float x, float y, const char* s, float ts,
                  float alpha, bool centred, bool dim) {
    if (!e->font.ok || !s || !*s) return;
    float gt, gb, yo = y;
    if (textBounds(e->font.set, s, ts, &gt, &gb)) yo = y - (gt + gb) * 0.5f;
    float xo = x;
    if (centred) xo = x - measureText(e, s, ts) * 0.5f;
    float o[3] = {c[0] + r[0]*xo + up[0]*yo,
                  c[1] + r[1]*xo + up[1]*yo,
                  c[2] + r[2]*xo + up[2]*yo};
    o[0] -= c[0] * 0.010f; o[1] -= c[1] * 0.010f; o[2] -= c[2] * 0.010f;
    const float* tc = dim ? kPalTextDim : kPalText;
    glUseProgram(e->textProg);
    glUniformMatrix4fv(glGetUniformLocation(e->textProg, "uMVP"),
                       1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                tc[0], tc[1], tc[2]);
    glUniform1i(glGetUniformLocation(e->textProg, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, e->font.tex);
    drawTextPanel(e, s, o, r, up, ts, 0.0f, alpha);
    glUseProgram(e->shapeProg);
}

// a rounded quad centred at card-local (x,y); clipY/clipC/clipH fade it
// at the pane's scroll-band edges like the grid's cells
static void sQuad(HudEngine* e, const Mat4& vp, const float c[3],
                  const float r[3], const float up[3],
                  float x, float y, float hw, float hh, float rad,
                  const float col[4],
                  float clipY = 0.0f, float clipC = 0.0f,
                  float clipH = 0.0f) {
    const float qc[3] = {c[0] + r[0]*x + up[0]*y,
                         c[1] + r[1]*x + up[1]*y,
                         c[2] + r[2]*x + up[2]*y};
    shapeQuad(e, vp, qc, r, up, 0.006f, 0.0f, hw, hh, hw, hh, rad, 0.0f,
              0.002f, col, -1.0f, 0.0f, clipY, clipC, clipH);
}

static const char* infoText(HudEngine* e, int txt) {
    SetOverlay& st = e->set;
    const std::string& s = st.snap.txt[txt];
    if (txt == TX_SSID && s.empty())
        return e->uiQs[UIQS_NOTCONN].empty() ? "Not connected"
                                             : e->uiQs[UIQS_NOTCONN].c_str();
    if (txt == TX_ENV)
        return s.empty() ? setStr(e, UISET_ENV_PT)
             : s == "passthrough" ? setStr(e, UISET_ENV_PT)
             : s == "builtin" ? setStr(e, UISET_ENV_BUILTIN)
             : s.c_str();
    return s.empty() ? setStr(e, UISET_UNKNOWN) : s.c_str();
}

static float sliderNorm(const SetOverlay& st, int id) {
    if (id == SL_IPD)
        return (st.snap.sld[SL_IPD] - kIpdMinMm) / (kIpdMaxMm - kIpdMinMm);
    return st.snap.sld[id];
}

static void drawSliderRow(HudEngine* e, const Mat4& vp, const float c[3],
                          const float r[3], const float up[3],
                          const SetRow& row, float cy, float gs, float ga,
                          bool hot) {
    SetOverlay& st = e->set;
    float tl, tr;
    setSliderTrack(&tl, &tr);
    const float v = sliderNorm(st, row.id);
    const float tc[3] = {c[0] + r[0]*((tl+tr)*0.5f)*gs + up[0]*cy*gs,
                         c[1] + r[1]*((tl+tr)*0.5f)*gs + up[1]*cy*gs,
                         c[2] + r[2]*((tl+tr)*0.5f)*gs + up[2]*cy*gs};
    const float trackHW = (tr - tl) * 0.5f * gs;
    const float tcol[4] = {kPalSurfaceHigh[0], kPalSurfaceHigh[1],
                           kPalSurfaceHigh[2], 0.65f * ga};
    shapeQuad(e, vp, tc, r, up, 0.006f, 0.0f, trackHW,
              kQuickTrackHH * gs, trackHW, kQuickTrackHH * gs,
              kQuickTrackHH * gs, 0.0f, 0.0018f, tcol);
    const float fw = (tr - tl) * v;
    if (fw > 0.001f) {
        const float fc[3] = {c[0] + r[0]*(tl + fw*0.5f)*gs + up[0]*cy*gs,
                             c[1] + r[1]*(tl + fw*0.5f)*gs + up[1]*cy*gs,
                             c[2] + r[2]*(tl + fw*0.5f)*gs + up[2]*cy*gs};
        const float fcol[4] = {kPalAccent[0], kPalAccent[1], kPalAccent[2],
                               0.90f * ga};
        const float fhw = fw * 0.5f * gs;
        shapeQuad(e, vp, fc, r, up, 0.007f, 0.0f, fhw, kQuickTrackHH * gs,
                  fhw, kQuickTrackHH * gs, kQuickTrackHH * gs, 0.0f,
                  0.0018f, fcol);
    }
    const float kx = (tl + (tr - tl) * v) * gs;
    const float kc[3] = {c[0] + r[0]*kx + up[0]*cy*gs,
                         c[1] + r[1]*kx + up[1]*cy*gs,
                         c[2] + r[2]*kx + up[2]*cy*gs};
    const float kr = (hot ? kQuickKnobR * 1.15f : kQuickKnobR) * gs;
    const float kcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           (hot ? 1.0f : 0.92f) * ga};
    shapeQuad(e, vp, kc, r, up, 0.008f, 0.0f, kr, kr, kr, kr, kr,
              0.0f, 0.0018f, kcol);
    char val[24];
    if (row.id == SL_IPD) snprintf(val, sizeof(val), "%.1f mm",
                                   st.snap.sld[SL_IPD]);
    else snprintf(val, sizeof(val), "%d%%", (int)(v * 100.0f + 0.5f));
    const float vw = measureText(e, val, 0.0012f * gs);
    sText(e, vp, c, r, up, tl * gs - 0.012f * gs - vw, cy * gs, val,
          0.0012f * gs, ga, false, true);
}

static void drawSwitch(HudEngine* e, const Mat4& vp, const float c[3],
                       const float r[3], const float up[3],
                       float x, float y, bool on, float gs, float ga) {
    const float tw = 0.036f * gs, th = 0.020f * gs;
    const float tc[3] = {c[0] + r[0]*x + up[0]*y,
                         c[1] + r[1]*x + up[1]*y,
                         c[2] + r[2]*x + up[2]*y};
    const float* bg = on ? kPalAccent : kPalSurfaceHigh;
    const float bcol[4] = {bg[0], bg[1], bg[2], (on ? 0.85f : 0.7f) * ga};
    shapeQuad(e, vp, tc, r, up, 0.006f, 0.0f, tw * 0.5f, th * 0.5f,
              tw * 0.5f, th * 0.5f, th * 0.5f, 0.0f, 0.0016f, bcol);
    const float kx = x + (on ? tw * 0.30f : -tw * 0.30f);
    const float kc[3] = {c[0] + r[0]*kx + up[0]*y,
                         c[1] + r[1]*kx + up[1]*y,
                         c[2] + r[2]*kx + up[2]*y};
    const float kcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           0.94f * ga};
    const float kr = th * 0.36f;
    shapeQuad(e, vp, kc, r, up, 0.008f, 0.0f, kr, kr, kr, kr, kr,
              0.0f, 0.0016f, kcol);
}

static void drawRowBody(HudEngine* e, const Mat4& vp, const float c[3],
                        const float r[3], const float up[3],
                        int i, float gs, float ga) {
    SetOverlay& st = e->set;
    const SetRow& row = st.rows[i];
    float rx, ry, hw, hh;
    setRowRect(st, i, &rx, &ry, &hw, &hh);
    if (!setRowShown(st, i)) return;
    float blo, bhi;
    setClipBand(&blo, &bhi);
    const float clipC = (blo + bhi) * 0.5f * gs;
    const float clipH = (bhi - blo) * 0.5f * gs;
    const bool full = ry + hh <= bhi + 0.002f &&
                      ry - hh >= blo - 0.002f;
    const bool en = setImplemented(row.aux);
    const bool hot = st.zone == SZONE_ROW && st.hover == i &&
                     st.dlg == DLG_NONE;
    const float fade = en ? 1.0f : 0.45f;
    const float paneLx = rx - hw;
    // row surface: brighter under the cursor, fades at the band edges
    const float hov = hot ? 0.16f : 0.0f;
    const float rcol[4] = {kPalSurface[0], kPalSurface[1], kPalSurface[2],
                           (0.35f + hov) * fade * ga};
    const float rcx = rx * gs, rcy = ry * gs;
    sQuad(e, vp, c, r, up, rcx, rcy, hw * gs, hh * gs, 0.018f * gs, rcol,
          rcy, clipC, clipH);
    if (!full) return;   // contents only draw while the band holds them
    const float lxC = paneLx + 0.016f;
    const float lx = lxC * gs;
    const float reC = rx + hw;              // the row's right edge
    auto fit = [&](const char* str, float ts, float xC,
                   float rightC) -> std::string {
        return clipText(e->font.set, str, ts, (rightC - xC) * gs);
    };
    // where the row's texts have to stop: before the right-side control
    float txR = reC - 0.016f;
    switch (row.kind) {
    case RK_TOGGLE: txR = reC - 0.070f; break;
    case RK_ACTION: txR = reC - 0.055f; break;
    case RK_SLIDER: {
        float tl, tr;
        setSliderTrack(&tl, &tr);
        txR = tl - 0.070f;
        break;
    }
    case RK_SEG:
        txR = reC - 0.024f - setSegCount(row.id) * kSetSegCellW;
        break;
    case RK_SCANHDR:
        txR = row.id == SEC_CTRL ? reC - 0.016f
                                 : setScanPillL() - 0.012f;
        break;
    case RK_NET: case RK_ENV: txR = reC - 0.055f; break;
    default: break;
    }
    const char* label = row.aux >= 0 && row.aux < RID_COUNT
                        ? setStr(e, UISET_LBL0 + row.aux) : "";
    const char* desc = row.aux >= 0 && row.aux < RID_COUNT
                       ? setStr(e, UISET_DESC0 + row.aux) : "";
    std::string infoVal;
    float infoW = 0.0f;
    if (row.kind == RK_INFO) {
        infoVal = fit(infoText(e, row.id), 0.0014f * gs, lxC,
                      reC - 0.016f);
        infoW = measureText(e, infoVal.c_str(), 0.0014f * gs);
        txR = reC - 0.016f - infoW - 0.030f;
    }
    switch (row.kind) {
    case RK_TOGGLE: case RK_ACTION: case RK_INFO: case RK_SEG: {
        const std::string t = fit(label, 0.0016f * gs, lxC, txR);
        const std::string d = fit(desc, 0.0011f * gs, lxC, txR);
        sText(e, vp, c, r, up, lx, rcy + 0.016f * gs, t.c_str(),
              0.0016f * gs, fade * ga, false, false);
        sText(e, vp, c, r, up, lx, rcy - 0.022f * gs, d.c_str(),
              0.0011f * gs, fade * ga, false, true);
        break;
    }
    default:
        break;
    }
    switch (row.kind) {
    case RK_TOGGLE:
        drawSwitch(e, vp, c, r, up,
                   (rx + hw - 0.035f) * gs, rcy,
                   (st.snap.toggles >> row.id) & 1u, gs, fade * ga);
        break;
    case RK_SLIDER: {
        const std::string t = fit(label, 0.0016f * gs, lxC, txR);
        const std::string d = fit(desc, 0.0011f * gs, lxC, txR);
        sText(e, vp, c, r, up, lx, rcy + 0.018f * gs, t.c_str(),
              0.0016f * gs, ga, false, false);
        sText(e, vp, c, r, up, lx, rcy - 0.024f * gs, d.c_str(),
              0.0011f * gs, ga, false, true);
        drawSliderRow(e, vp, c, r, up, row, ry, gs, ga,
                      st.slideHeld == i ||
                      (st.zone == SZONE_SLIDER && st.hover == i));
        break;
    }
    case RK_INFO:
        sText(e, vp, c, r, up, (reC - 0.016f) * gs - infoW, rcy,
              infoVal.c_str(), 0.0014f * gs, ga, false, true);
        break;
    case RK_ACTION: {
        const float ic[3] = {c[0] + r[0]*(rx + hw - 0.030f)*gs + up[0]*rcy,
                             c[1] + r[1]*(rx + hw - 0.030f)*gs + up[1]*rcy,
                             c[2] + r[2]*(rx + hw - 0.030f)*gs + up[2]*rcy};
        const float icol[4] = {kPalTextDim[0], kPalTextDim[1],
                               kPalTextDim[2], fade * ga};
        setGlyph(e, vp, SG_CHEVRON, ic, r, up, 0.020f * gs, icol);
        break;
    }
    case RK_SEG: {
        const int n = setSegCount(row.id);
        const int cur = setSegCurrent(row.id, st.snap);
        for (int o = 0; o < n; ++o) {
            float sx, sy, shw, shh;
            setSegRect(row.id, o, ry, &sx, &sy, &shw, &shh);
            const bool sel = o == cur;
            const bool shov = st.zone == SZONE_SEG && st.hover == i &&
                              st.aux == o;
            const float* bg = sel ? kPalAccent : kPalSurfaceHigh;
            const float scol[4] = {bg[0], bg[1], bg[2],
                                   (sel ? 0.9f : (shov ? 0.75f : 0.55f)) *
                                   fade * ga};
            sQuad(e, vp, c, r, up, sx * gs, sy * gs, shw * gs, shh * gs,
                  shh * 0.9f * gs, scol);
            const int strId = row.id == PK_THEME
                ? UISET_THEME_DARK + o
                : row.id == PK_LANG ? UISET_LANG_EN + o
                : UISET_3DOF + o;
            const std::string t = fit(setStr(e, strId), 0.0011f * gs,
                                      sx - shw, sx + shw);
            sText(e, vp, c, r, up, sx * gs, sy * gs, t.c_str(),
                  0.0011f * gs, fade * ga, true, !sel);
        }
        break;
    }
    case RK_SCANHDR: {
        const bool scanning =
            (row.id == SEC_WIFI && (st.snap.flags & SFL_WIFI_SCAN)) ||
            (row.id == SEC_BT && (st.snap.flags & SFL_BT_SCAN)) ||
            (row.id == SEC_CTRL && (st.snap.flags & SFL_CTRL_SCAN));
        const int idleStr = row.id == SEC_WIFI ? UISET_IDLE_WIFI
                          : row.id == SEC_BT ? UISET_IDLE_BT
                          : row.id == SEC_CTRL ? UISET_IDLE_CTRL
                          : -1;
        const int scanStr = row.id == SEC_WIFI ? UISET_SCAN_WIFI
                          : row.id == SEC_BT ? UISET_SCAN_BT
                          : UISET_SCAN_CTRL;
        const float ic[3] = {c[0] + r[0]*(paneLx + 0.030f)*gs + up[0]*rcy,
                             c[1] + r[1]*(paneLx + 0.030f)*gs + up[1]*rcy,
                             c[2] + r[2]*(paneLx + 0.030f)*gs + up[2]*rcy};
        const float icol[4] = {kPalAccent[0], kPalAccent[1],
                               kPalAccent[2], ga};
        setGlyph(e, vp, SG_REFRESH, ic, r, up, 0.020f * gs, icol);
        const std::string t = fit(label, 0.0016f * gs, lxC + 0.040f, txR);
        const std::string d = fit(
            setStr(e, scanning ? scanStr : idleStr >= 0 ? idleStr
                                                          : UISET_IDLE_WIFI),
            0.0011f * gs, lxC + 0.040f, txR);
        sText(e, vp, c, r, up, lx + 0.040f * gs, rcy + 0.016f * gs,
              t.c_str(), 0.0016f * gs, ga, false, false);
        sText(e, vp, c, r, up, lx + 0.040f * gs, rcy - 0.022f * gs,
              d.c_str(), 0.0011f * gs, ga, false,
              scanning ? false : true);
        if (row.id != SEC_CTRL) {
            const char* rt = setStr(e, UISET_REFRESH);
            float bx, phw;
            setScanPill(measureText(e, rt, 0.0011f * gs) / gs, &bx, &phw);
            const bool bhov = st.zone == SZONE_SCAN && st.hover == i;
            const float bcol[4] = {kPalSurfaceHigh[0], kPalSurfaceHigh[1],
                                   kPalSurfaceHigh[2],
                                   (bhov ? 0.85f : 0.6f) * ga};
            sQuad(e, vp, c, r, up, bx * gs, rcy, phw * gs, 0.026f * gs,
                  0.026f * gs, bcol);
            const std::string pt = fit(rt, 0.0011f * gs,
                                       bx - phw + kSetPillPadX,
                                       bx + phw - kSetPillPadX);
            sText(e, vp, c, r, up, bx * gs, rcy, pt.c_str(),
                  0.0011f * gs, ga, true, false);
        }
        break;
    }
    case RK_NET: {
        const SetWifi& n = st.snap.wifi[row.aux];
        const float ic[3] = {c[0] + r[0]*(paneLx + 0.028f)*gs + up[0]*rcy,
                             c[1] + r[1]*(paneLx + 0.028f)*gs + up[1]*rcy,
                             c[2] + r[2]*(paneLx + 0.028f)*gs + up[2]*rcy};
        const float* icp = n.conn ? kPalAccent : kPalTextDim;
        const float icol[4] = {icp[0], icp[1], icp[2], ga};
        setSecIcon(e, vp, SEC_WIFI, ic, r, up, 0.018f * gs, icol);
        const std::string t = fit(n.ssid.c_str(), 0.0014f * gs,
                                  lxC + 0.036f, txR);
        const char* sub = n.conn ? setStr(e, UISET_CONNECTED)
                        : n.saved >= 0 ? setStr(e, UISET_SAVED)
                        : n.sec == WSEC_OPEN ? setStr(e, UISET_OPENNET)
                        : setStr(e, UISET_SECURED);
        const std::string d = fit(sub, 0.00105f * gs, lxC + 0.036f, txR);
        sText(e, vp, c, r, up, lx + 0.036f * gs, rcy + 0.014f * gs,
              t.c_str(), 0.0014f * gs, ga, false, false);
        sText(e, vp, c, r, up, lx + 0.036f * gs, rcy - 0.020f * gs,
              d.c_str(), 0.00105f * gs, ga, false, !n.conn);
        if (n.sec != WSEC_OPEN) {
            const float lc[3] = {c[0] + r[0]*(rx + hw - 0.036f)*gs +
                                     up[0]*rcy,
                                 c[1] + r[1]*(rx + hw - 0.036f)*gs +
                                     up[1]*rcy,
                                 c[2] + r[2]*(rx + hw - 0.036f)*gs +
                                     up[2]*rcy};
            const float lcol[4] = {kPalTextDim[0], kPalTextDim[1],
                                   kPalTextDim[2], ga};
            setGlyph(e, vp, SG_LOCK, lc, r, up, 0.016f * gs, lcol);
        }
        break;
    }
    case RK_BTROW: {
        const SetBtDev& d = st.snap.bt[row.aux];
        const float ic[3] = {c[0] + r[0]*(paneLx + 0.028f)*gs + up[0]*rcy,
                             c[1] + r[1]*(paneLx + 0.028f)*gs + up[1]*rcy,
                             c[2] + r[2]*(paneLx + 0.028f)*gs + up[2]*rcy};
        const float* icp = d.conn ? kPalAccent : kPalTextDim;
        const float icol[4] = {icp[0], icp[1], icp[2], ga};
        setSecIcon(e, vp, SEC_BT, ic, r, up, 0.016f * gs, icol);
        const float pairR = d.bonded ? txR :
            reC - 0.016f - measureText(e, setStr(e, UISET_PAIR),
                                       0.0012f * gs) / gs - 0.014f;
        const std::string t = fit(
            d.name.empty() ? d.addr.c_str() : d.name.c_str(),
            0.0014f * gs, lxC + 0.036f, pairR);
        const char* sub = d.conn ? setStr(e, UISET_CONNECTED)
                        : d.bonded ? setStr(e, UISET_PAIRED)
                        : d.addr.c_str();
        const std::string sd = fit(sub, 0.00105f * gs, lxC + 0.036f,
                                   pairR);
        sText(e, vp, c, r, up, lx + 0.036f * gs, rcy + 0.014f * gs,
              t.c_str(), 0.0014f * gs, ga, false, false);
        sText(e, vp, c, r, up, lx + 0.036f * gs, rcy - 0.020f * gs,
              sd.c_str(), 0.00105f * gs, ga, false, !d.conn);
        if (!d.bonded)
            sText(e, vp, c, r, up,
                  (reC - 0.016f) * gs -
                  measureText(e, setStr(e, UISET_PAIR), 0.0012f * gs),
                  rcy, setStr(e, UISET_PAIR), 0.0012f * gs, ga, false,
                  false);
        break;
    }
    case RK_IME: {
        const SetImeOpt& im = st.snap.imes[row.aux];
        const std::string t = fit(im.label.c_str(), 0.0014f * gs, lxC,
                                  txR);
        sText(e, vp, c, r, up, lx, rcy + 0.014f * gs, t.c_str(),
              0.0014f * gs, ga, false, false);
        if (im.active)
            sText(e, vp, c, r, up, lx, rcy - 0.020f * gs,
                  setStr(e, UISET_IME_ACTIVE), 0.00105f * gs, ga, false,
                  false);
        if (im.active) {
            const float ic[3] = {c[0] + r[0]*(rx + hw - 0.030f)*gs +
                                     up[0]*rcy,
                                 c[1] + r[1]*(rx + hw - 0.030f)*gs +
                                     up[1]*rcy,
                                 c[2] + r[2]*(rx + hw - 0.030f)*gs +
                                     up[2]*rcy};
            const float icol[4] = {kPalAccent[0], kPalAccent[1],
                                   kPalAccent[2], ga};
            setGlyph(e, vp, SG_CHECK, ic, r, up, 0.018f * gs, icol);
        }
        break;
    }
    case RK_ENV: {
        const bool active = setEnvActive(st.snap, row.aux);
        const char* name = row.aux == 0 ? setStr(e, UISET_ENV_PT)
                         : row.aux == 1 ? setStr(e, UISET_ENV_BUILTIN)
                         : st.snap.envs[row.aux - 2].name.empty()
                           ? st.snap.envs[row.aux - 2].id.c_str()
                           : st.snap.envs[row.aux - 2].name.c_str();
        const char* sub = row.aux == 0 ? setStr(e, UISET_ENV_PT_D)
                        : row.aux == 1 ? setStr(e, UISET_ENV_BUILTIN_D)
                        : st.snap.envs[row.aux - 2].st == 1
                          ? setStr(e, UISET_ENV_BAD)
                        : st.snap.envs[row.aux - 2].st == 2
                          ? setStr(e, UISET_ENV_NOMAP)
                          : st.snap.envs[row.aux - 2].id.c_str();
        const float* np = active ? kPalAccent : kPalText;
        const float ncol[4] = {np[0], np[1], np[2], ga};
        glUseProgram(e->textProg);
        glUniform3f(glGetUniformLocation(e->textProg, "uColor"),
                    np[0], np[1], np[2]);
        (void)ncol;
        glUseProgram(e->shapeProg);
        const std::string t = fit(name, 0.0014f * gs, lxC, txR);
        const std::string d = fit(sub, 0.00105f * gs, lxC, txR);
        sText(e, vp, c, r, up, lx, rcy + 0.014f * gs, t.c_str(),
              0.0014f * gs, ga, false, !active);
        sText(e, vp, c, r, up, lx, rcy - 0.020f * gs, d.c_str(),
              0.00105f * gs, ga, false, true);
        if (active) {
            const float ic[3] = {c[0] + r[0]*(rx + hw - 0.030f)*gs +
                                     up[0]*rcy,
                                 c[1] + r[1]*(rx + hw - 0.030f)*gs +
                                     up[1]*rcy,
                                 c[2] + r[2]*(rx + hw - 0.030f)*gs +
                                     up[2]*rcy};
            const float icol[4] = {kPalAccent[0], kPalAccent[1],
                                   kPalAccent[2], ga};
            setGlyph(e, vp, SG_CHECK, ic, r, up, 0.018f * gs, icol);
        }
        if (row.aux >= 2) {
            const float tc[3] = {c[0] + r[0]*(rx + hw - 0.026f)*gs +
                                     up[0]*rcy,
                                 c[1] + r[1]*(rx + hw - 0.026f)*gs +
                                     up[1]*rcy,
                                 c[2] + r[2]*(rx + hw - 0.026f)*gs +
                                     up[2]*rcy};
            const bool dhov = st.zone == SZONE_DEL && st.hover == i;
            const float* dp = dhov ? kPalDanger : kPalTextDim;
            const float dcol[4] = {dp[0], dp[1], dp[2], ga};
            setGlyph(e, vp, SG_TRASH, tc, r, up, 0.017f * gs, dcol);
        }
        break;
    }
    case RK_CTRL: {
        const SetCtrl& ct = st.snap.ctrl[row.aux];
        const float battR = ct.batt >= 0 ? reC - 0.11f : txR;
        const std::string t = fit(
            setStr(e, UISET_LBL0 + (row.aux == 0 ? RID_CTRL_L
                                                : RID_CTRL_R)),
            0.0014f * gs, lxC, battR);
        sText(e, vp, c, r, up, lx, rcy + 0.014f * gs, t.c_str(),
              0.0014f * gs, ga, false, false);
        const int stateStr = ct.state == 1 ? UISET_CONNECTED
                           : ct.state == 2 ? UISET_PAIRING
                           : UISET_DISCONNECTED;
        const std::string d = fit(setStr(e, stateStr), 0.00105f * gs,
                                  lxC, battR);
        sText(e, vp, c, r, up, lx, rcy - 0.020f * gs, d.c_str(),
              0.00105f * gs, ga, false, ct.state != 1);
        if (ct.batt >= 0) {
            char b[24];
            snprintf(b, sizeof(b), setStr(e, UISET_CTRL_BATT_FMT),
                     ct.batt);
            const float bw = measureText(e, b, 0.0011f * gs);
            sText(e, vp, c, r, up, (reC - 0.016f) * gs - bw, rcy, b,
                  0.0011f * gs, ga, false, true);
        }
        break;
    }
    case RK_BRAND: {
        const float my = rcy + 0.031f * gs;
        const float ic[3] = {c[0] + r[0]*rcx + up[0]*my,
                             c[1] + r[1]*rcx + up[1]*my,
                             c[2] + r[2]*rcx + up[2]*my};
        setBrandMark(e, vp, ic, r, up, 0.020f * gs, ga);
        const std::string t = fit(setStr(e, UISET_LBL0 + RID_BRAND),
                                  0.0027f * gs, rx - hw, rx + hw);
        const std::string d = fit(setStr(e, UISET_DESC0 + RID_BRAND),
                                  0.0013f * gs, rx - hw, rx + hw);
        sText(e, vp, c, r, up, rcx, rcy - 0.009f * gs, t.c_str(),
              0.0027f * gs, ga, true, false);
        sText(e, vp, c, r, up, rcx, rcy - 0.043f * gs, d.c_str(),
              0.0013f * gs, ga, true, true);
        break;
    }
    default:
        break;
    }
    // the empty-list hint lives in the free pane space under the scan
    // row, never inside the row's own band
    if (row.kind == RK_SCANHDR &&
        ((row.id == SEC_WIFI && st.snap.wifi.empty()) ||
         (row.id == SEC_BT && st.snap.bt.empty()))) {
        const float hy = rcy - hh * gs - 0.040f * gs;
        if (hy > blo * gs) {
            const std::string t = fit(
                setStr(e, row.id == SEC_WIFI ? UISET_EMPTY_WIFI
                                             : UISET_EMPTY_BT),
                0.0011f * gs, lxC, txR);
            sText(e, vp, c, r, up, lx, hy, t.c_str(), 0.0011f * gs, ga,
                  false, true);
        }
    }
}

static void drawDlg(HudEngine* e, const Mat4& vp, const float c[3],
                    const float r[3], const float up[3],
                    float gs, float ga) {
    SetOverlay& st = e->set;
    const bool kb = st.dlg == DLG_WIFI && st.dlgArg >= 0 &&
                    st.dlgArg < (int)st.snap.wifi.size() &&
                    setWifiNeedsPw(st.snap.wifi[st.dlgArg]);
    float dx, dy, dhw, dhh;
    setDlgRect(st.dlg, kb, &dx, &dy, &dhw, &dhh);
    const float dcol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                           0.95f * ga};
    sQuad(e, vp, c, r, up, dx * gs, dy * gs, dhw * gs, dhh * gs,
          0.05f * gs, dcol);
    const float rim[4] = {kPalText[0], kPalText[1], kPalText[2],
                          0.14f * ga};
    const float qc[3] = {c[0] + r[0]*dx*gs + up[0]*dy*gs,
                         c[1] + r[1]*dx*gs + up[1]*dy*gs,
                         c[2] + r[2]*dx*gs + up[2]*dy*gs};
    shapeQuad(e, vp, qc, r, up, 0.005f, 0.0f, dhw * gs, dhh * gs,
              dhw * gs, dhh * gs, 0.05f * gs, 0.0016f, 0.0012f, rim);
    const float ty = (dy + dhh - 0.040f) * gs;
    char title[160];
    title[0] = 0;
    if (st.dlg == DLG_WIFI && st.dlgArg >= 0 &&
        st.dlgArg < (int)st.snap.wifi.size())
        snprintf(title, sizeof(title), setStr(e, UISET_JOIN_FMT),
                 st.snap.wifi[st.dlgArg].ssid.c_str());
    else if (st.dlg == DLG_BT && st.dlgArg >= 0 &&
             st.dlgArg < (int)st.snap.bt.size())
        snprintf(title, sizeof(title), "%s",
                 st.snap.bt[st.dlgArg].name.empty()
                     ? st.snap.bt[st.dlgArg].addr.c_str()
                     : st.snap.bt[st.dlgArg].name.c_str());
    else if (st.dlg == DLG_ENV && st.dlgArg >= 2 &&
             st.dlgArg - 2 < (int)st.snap.envs.size())
        snprintf(title, sizeof(title), setStr(e, UISET_REMOVE_FMT),
                 st.snap.envs[st.dlgArg - 2].id.c_str());
    else if (st.dlg == DLG_REBOOT)
        snprintf(title, sizeof(title), "%s", setStr(e, UISET_REBOOT_T));
    sText(e, vp, c, r, up, dx * gs, ty, title, 0.0016f * gs, ga, true,
          false);
    if (st.dlg == DLG_BT) {
        const SetBtDev& d = st.snap.bt[st.dlgArg];
        sText(e, vp, c, r, up, dx * gs, ty - 0.040f * gs, d.addr.c_str(),
              0.0012f * gs, ga, true, true);
    } else if (st.dlg == DLG_ENV) {
        sText(e, vp, c, r, up, dx * gs, ty - 0.040f * gs,
              setStr(e, UISET_REMOVE_BODY), 0.0012f * gs, ga, true, true);
    } else if (st.dlg == DLG_REBOOT) {
        sText(e, vp, c, r, up, dx * gs, ty - 0.040f * gs,
              setStr(e, UISET_REBOOT_B), 0.0012f * gs, ga, true, true);
    }
    if (kb) {
        // password field: dots + a caret
        float fx, fy, fhw, fhh;
        setDlgFieldRect(&fx, &fy, &fhw, &fhh);
        const float fcol[4] = {kPalSurface[0], kPalSurface[1],
                               kPalSurface[2], 0.7f * ga};
        sQuad(e, vp, c, r, up, fx * gs, fy * gs, fhw * gs, fhh * gs,
              fhh * 0.6f * gs, fcol);
        std::string dots(st.edit.size(), '\xE2');
        dots.clear();
        for (size_t i = 0; i < st.edit.size(); ++i) dots += '\xE2',
            dots += '\x80', dots += '\xA2';   // U+2022 bullet
        if (st.edit.empty())
            sText(e, vp, c, r, up, (fx - fhw + 0.014f) * gs, fy * gs,
                  setStr(e, UISET_PASSWORD), 0.0013f * gs, ga * 0.6f,
                  false, true);
        else
            sText(e, vp, c, r, up, (fx - fhw + 0.014f) * gs, fy * gs,
                  dots.c_str(), 0.0013f * gs, ga, false, false);
        for (int i = 0; i < setKbdCount(st.kbdPage); ++i) {
            float kx, ky, khw, khh;
            setKeyRect(st.kbdPage, i, &kx, &ky, &khw, &khh);
            const bool khov = st.zone == SZONE_KEY && st.hover == i;
            const float kcol[4] = {kPalSurfaceHigh[0], kPalSurfaceHigh[1],
                                   kPalSurfaceHigh[2],
                                   (khov ? 0.85f : 0.55f) * ga};
            sQuad(e, vp, c, r, up, kx * gs, ky * gs, khw * gs, khh * gs,
                  0.010f * gs, kcol);
            char ch[2] = {(char)setKeyCode(st.kbdPage, i, st.kbdShift), 0};
            const char* kl = setKeyLabel(st.kbdPage, i);
            const char* txt = *kl ? kl : (ch[0] > 0 ? ch : "");
            if (!strcmp(kl, "space")) txt = setStr(e, UISET_SPACE);
            sText(e, vp, c, r, up, kx * gs, ky * gs, txt,
                  0.0012f * gs, ga, true, false);
        }
    }
    // buttons: forget(left) / cancel(mid) / confirm(right)
    const int nb = setDlgBtnCount(st.dlg, st);
    for (int i = 0; i < nb; ++i) {
        const int slot = nb == 3 ? i : i + 1;
        float bx, by, bhw, bhh;
        setDlgBtnRect(st.dlg, kb, slot, &bx, &by, &bhw, &bhh);
        const bool bhov = st.zone == SZONE_BTN && st.hover == i;
        const bool danger = i == 0 && nb == 3;
        const bool confirm = i == nb - 1 && nb > 1;
        const float* bg = danger ? kPalDanger
                        : confirm ? kPalAccent : kPalSurfaceHigh;
        const float bcol[4] = {bg[0], bg[1], bg[2],
                               (bhov ? 0.9f : (confirm || danger ? 0.75f
                                                                : 0.55f)) *
                               ga};
        sQuad(e, vp, c, r, up, bx * gs, by * gs, bhw * gs, bhh * gs,
              bhh * 0.9f * gs, bcol);
        const int strId = i == 0 && nb == 3
            ? (st.dlg == DLG_ENV ? UISET_REMOVE : UISET_FORGET)
            : i == nb - 1
              ? (st.dlg == DLG_WIFI ? UISET_CONNECT
                 : st.dlg == DLG_ENV ? UISET_REMOVE
                 : st.dlg == DLG_REBOOT ? UISET_REBOOT : UISET_FORGET)
              : UISET_CANCEL;
        sText(e, vp, c, r, up, bx * gs, by * gs, setStr(e, strId),
              0.0012f * gs, ga, true, false);
    }
}

void drawSet(HudEngine* e, const Mat4& vp) {
    SetOverlay& st = e->set;
    if (st.openT <= 0.0f) return;
    const float qt = easeOutCubic(st.openT);
    const float gs = kSetScale0 + (1.0f - kSetScale0) * qt;
    const float ga = qt;
    float c[3], r[3], up[3];
    setCenter(e->dockYaw, ringPitchFor(e->dockPitch), e->ringPos, c, r, up);
    const float hw = kSetHW * gs, hh = kSetHH * gs;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glUseProgram(e->shapeProg);

    const float shc[3] = {c[0] - up[0] * 0.024f * gs,
                          c[1] - up[1] * 0.024f * gs,
                          c[2] - up[2] * 0.024f * gs};
    const float shCol[4] = {0.0f, 0.0f, 0.0f, 0.34f * ga};
    shapeQuad(e, vp, shc, r, up, -0.03f, 0.0f, hw + 0.06f, hh + 0.06f,
              hw, hh, 0.06f, -1.0f, 0.06f, shCol);
    const float cardCol[4] = {kPalPanel[0], kPalPanel[1], kPalPanel[2],
                              0.66f * ga};
    shapeQuad(e, vp, c, r, up, 0.004f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0f, 0.003f, cardCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.14f);
    const float rimCol[4] = {kPalText[0], kPalText[1], kPalText[2],
                             0.14f * ga};
    shapeQuad(e, vp, c, r, up, 0.005f, 0.0f, hw, hh, hw, hh, 0.06f,
              0.0016f, 0.0012f, rimCol, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
              0.30f);

    // title band + close disc
    const float hy = setTitleY() * gs;
    const bool bhov = st.zone == SZONE_CLOSE;
    const float bx = setCloseX() * gs;
    const float bc[3] = {c[0] + r[0]*bx + up[0]*hy,
                         c[1] + r[1]*bx + up[1]*hy,
                         c[2] + r[2]*bx + up[2]*hy};
    const float* bgp = bhov ? kPalDanger : kPalText;
    const float bg[4] = {bgp[0], bgp[1], bgp[2],
                         (bhov ? 0.32f : 0.13f) * ga};
    shapeQuad(e, vp, bc, r, up, 0.006f, 0.0f, kSetCloseR * gs,
              kSetCloseR * gs, kSetCloseR * gs, kSetCloseR * gs,
              kSetCloseR * gs, 0.0f, 0.002f, bg);
    const float xcol[4] = {kPalText[0], kPalText[1], kPalText[2],
                           0.92f * ga};
    const float il = kSetCloseR * 0.52f * gs, it = 0.0026f;
    shapeQuad(e, vp, bc, r, up, 0.008f, 0.785398f, il, it, il, it, it,
              0.0f, 0.0015f, xcol);
    shapeQuad(e, vp, bc, r, up, 0.008f, -0.785398f, il, it, il, it, it,
              0.0f, 0.0015f, xcol);
    sText(e, vp, c, r, up, (-kSetHW + kSetPad) * gs, hy,
          setStr(e, UISET_TITLE), 0.0017f * gs, ga, false, false);

    // sidebar
    for (int i = 0; i < SEC_COUNT; ++i) {
        float sx, sy, shw, shh;
        setSecRect(i, &sx, &sy, &shw, &shh);
        const bool sel = i == st.section;
        const bool shov = st.zone == SZONE_SEC && st.hover == i;
        const float scol[4] = {kPalSurface[0], kPalSurface[1],
                               kPalSurface[2],
                               (sel ? 0.75f : shov ? 0.45f : 0.0f) * ga};
        if (sel || shov)
            sQuad(e, vp, c, r, up, sx * gs, sy * gs, shw * gs, shh * gs,
                  shh * 0.8f * gs, scol);
        const float ix = (sx - shw + 0.024f) * gs;
        const float ic[3] = {c[0] + r[0]*ix + up[0]*sy*gs,
                             c[1] + r[1]*ix + up[1]*sy*gs,
                             c[2] + r[2]*ix + up[2]*sy*gs};
        const float* icp = sel ? kPalText : kPalTextDim;
        const float icol[4] = {icp[0], icp[1], icp[2], ga};
        setSecIcon(e, vp, i, ic, r, up, 0.018f * gs, icol);
        const char* name = setStr(e, UISET_SEC0 + i);
        float ts = 0.0014f * gs;
        const float maxW = (setSideR() + kSetPad * 0.4f -
                            (sx - shw + 0.058f) - 0.010f) * gs;
        const float tw = measureText(e, name, ts);
        if (tw > maxW && tw > 0.0f) ts *= maxW / tw;
        sText(e, vp, c, r, up, ix + 0.034f * gs, sy * gs, name, ts,
              ga, false, !sel);
    }
    // sidebar divider: under the title band down to the pad
    const float dv = setSideR() + kSetPad * 0.4f;
    float dLo, dHi;
    setClipBand(&dLo, &dHi);
    const float dvc[3] = {c[0] + r[0]*dv*gs + up[0]*(dHi+dLo)*0.5f*gs,
                          c[1] + r[1]*dv*gs + up[1]*(dHi+dLo)*0.5f*gs,
                          c[2] + r[2]*dv*gs + up[2]*(dHi+dLo)*0.5f*gs};
    const float dvCol[4] = {kPalSurfaceHigh[0], kPalSurfaceHigh[1],
                            kPalSurfaceHigh[2], 0.5f * ga};
    const float dhh = (dHi - dLo) * 0.5f * gs;
    shapeQuad(e, vp, dvc, r, up, 0.006f, 0.0f, 0.0008f, dhh, 0.0008f,
              dhh, 0.0f, 0.0f, 0.001f, dvCol);

    for (int i = 0; i < (int)st.rows.size(); ++i)
        drawRowBody(e, vp, c, r, up, i, gs, ga);

    if (st.dlg != DLG_NONE) {
        // dim the card under the modal
        const float dim[4] = {0.0f, 0.0f, 0.0f, 0.40f * ga};
        shapeQuad(e, vp, c, r, up, 0.007f, 0.0f, hw, hh, hw, hh, 0.06f,
                  0.0f, 0.003f, dim);
        drawDlg(e, vp, c, r, up, gs, ga);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

#include "layout.h"

#include "../common/config.h"
#include "../dock/layout.h"
#include "../panels/layout.h"

#include <cmath>
#include <cstring>

// row heights by kind, card-local metres
static float rowH(int kind) {
    switch (kind) {
    case RK_SLIDER:  return kSetSliderH;
    case RK_SCANHDR: return kSetScanH;
    case RK_BRAND:   return kSetBrandH;
    case RK_NET:
    case RK_BTROW:
    case RK_IME:
    case RK_ENV:
    case RK_CTRL:    return kSetListH;
    default:         return kSetRowH;
    }
}

static float paneL() { return setSideR() + kSetPad; }
static float paneR() { return kSetHW - kSetPad; }

void setCenter(float yaw, float pitch, const float origin[3],
               float c[3], float r[3], float up[3]) {
    ringPoint(yaw, pitch, kSetDist, kPanelY, origin, c, r, up);
}

// ------------------------------------------------------------- catalog

void setBuildRows(int sec, bool android, const SetSnap& snap,
                  std::vector<SetRow>* out) {
    out->clear();
    auto row = [out](int kind, int id, int aux) {
        SetRow r;
        r.kind = kind; r.id = id; r.aux = aux;
        out->push_back(r);
    };
    switch (sec) {
    case SEC_WIFI:
        row(RK_TOGGLE, TG_WIFI, RID_WIFI_ON);
        row(RK_INFO, TX_SSID, RID_WIFI_SSID);
        row(RK_SCANHDR, SEC_WIFI, RID_WIFI_LIST);
        for (int i = 0; i < (int)snap.wifi.size(); ++i)
            row(RK_NET, -1, i);
        break;
    case SEC_BT:
        row(RK_TOGGLE, TG_BT, RID_BT_ON);
        row(RK_SCANHDR, SEC_BT, RID_BT_LIST);
        for (int i = 0; i < (int)snap.bt.size(); ++i)
            row(RK_BTROW, -1, i);
        break;
    case SEC_CTRL:
        row(RK_SCANHDR, SEC_CTRL, RID_CTRL_SCAN);
        row(RK_CTRL, -1, 0);
        row(RK_CTRL, -1, 1);
        row(RK_ACTION, ACT_CTRL_UNBIND, RID_CTRL_UNBIND);
        break;
    case SEC_DISP:
        row(RK_SLIDER, SL_BRI, RID_BRI);
        row(RK_SEG, PK_THEME, RID_THEME);
        if (!android) break;
        row(RK_SLIDER, SL_IPD, RID_IPD);
        row(RK_SEG, PK_DOF, RID_DOF);
        row(RK_TOGGLE, TG_NIGHT, RID_NIGHT);
        break;
    case SEC_ENV:
        row(RK_SCANHDR, SEC_ENV, RID_ENV_LIST);
        row(RK_ENV, -1, 0);
        row(RK_ENV, -1, 1);
        for (int i = 0; i < (int)snap.envs.size(); ++i)
            row(RK_ENV, -1, i + 2);
        break;
    case SEC_SOUND:
        row(RK_SLIDER, SL_VOL, RID_VOL);
        row(RK_TOGGLE, TG_MIC, RID_MIC);
        break;
    case SEC_LANG:
        row(RK_SEG, PK_LANG, RID_LANG);
        break;
    case SEC_TIME:
        row(RK_ACTION, ACT_TIMEZONE, RID_TZ);
        break;
    case SEC_IME:
        row(RK_SCANHDR, SEC_IME, RID_IME_LIST);
        for (int i = 0; i < (int)snap.imes.size(); ++i)
            row(RK_IME, -1, i);
        break;
    case SEC_DEV:
        row(RK_TOGGLE, TG_ADB, RID_ADB);
        row(RK_TOGGLE, TG_STAY, RID_STAY);
        row(RK_TOGGLE, TG_TOUCHES, RID_TOUCHES);
        row(RK_TOGGLE, TG_DEBUG_HUD, RID_DEBUGHUD);
        break;
    case SEC_XP:
        for (int i = 0; i < (int)snap.xp.size(); ++i)
            row(RK_XP, -1, i);
        break;
    case SEC_ABOUT:
        row(RK_BRAND, -1, RID_BRAND);
        row(RK_INFO, TX_MODEL, android ? RID_MODEL : RID_SYSTEM);
        if (android) row(RK_INFO, TX_ANDROID, RID_ANDROID);
        row(RK_INFO, TX_HBVER, RID_HBVER);
        break;
    default:
        break;
    }
}

// ------------------------------------------------------------- layout

// sidebar order on the wayland host: the sections bound to Android
// providers or HMD hardware - the controller link, locale write,
// timezone intent, input methods and the developer toggles - drop out
static const int kSecsDesk[] = {SEC_WIFI, SEC_BT, SEC_DISP, SEC_ENV,
                                SEC_SOUND, SEC_XP, SEC_ABOUT};

int setSecCount(bool android) {
    return android ? SEC_COUNT : (int)(sizeof(kSecsDesk) / sizeof(int));
}

int setSecAt(bool android, int i) {
    if (i < 0 || i >= setSecCount(android)) return -1;
    return android ? i : kSecsDesk[i];
}

int setSecSlot(bool android, int sec) {
    const int n = setSecCount(android);
    for (int i = 0; i < n; ++i)
        if (setSecAt(android, i) == sec) return i;
    return -1;
}

float setSideL() { return -kSetHW + kSetPad; }
float setSideR() { return setSideL() + kSetSideW; }

void setClipBand(float* lo, float* hi) {
    *lo = -kSetHH + kSetPad;
    *hi = kSetHH - kSetHeadH;
}

void setSecRect(int i, float* x, float* y, float* hw, float* hh) {
    *x = (setSideL() + setSideR()) * 0.5f;
    *hw = kSetSideW * 0.5f - 0.010f;
    *hh = kSetSecRowH * 0.5f - 0.004f;
    *y = kSetHH - kSetHeadH - 0.012f - kSetSecRowH * 0.5f -
         i * kSetSecRowH;
}

// row i's top edge in the scrolled strip's own frame
static float rowTop(const SetOverlay& ov, int i) {
    float top = kSetHH - kSetHeadH - 0.012f;
    for (int k = 0; k < i; ++k)
        top -= rowH(ov.rows[k].kind) + kSetRowGap;
    return top;
}

void setRowRect(const SetOverlay& ov, int i, float* x, float* y,
                float* hw, float* hh) {
    const float h = rowH(i < (int)ov.rows.size() ? ov.rows[i].kind
                                                 : RK_INFO);
    *x = (paneL() + paneR()) * 0.5f;
    *hw = (paneR() - paneL()) * 0.5f;
    *hh = h * 0.5f;
    *y = rowTop(ov, i) - h * 0.5f + ov.scroll;
}

bool setRowShown(const SetOverlay& ov, int i) {
    float x, y, hw, hh;
    setRowRect(ov, i, &x, &y, &hw, &hh);
    float lo, hi;
    setClipBand(&lo, &hi);
    return y + hh >= lo - 0.02f && y - hh <= hi + 0.02f;
}

static float contentH(const SetOverlay& ov) {
    float h = 0.012f;
    for (size_t i = 0; i < ov.rows.size(); ++i)
        h += rowH(ov.rows[i].kind) +
             (i + 1 < ov.rows.size() ? kSetRowGap : 0.0f);
    return h;
}

float setScrollMax(const SetOverlay& ov) {
    float lo, hi;
    setClipBand(&lo, &hi);
    const float over = contentH(ov) - (hi - lo);
    return over > 0.0f ? over : 0.0f;
}

float setClampScroll(const SetOverlay& ov, float scroll) {
    const float mx = setScrollMax(ov);
    return scroll < 0.0f ? 0.0f : scroll > mx ? mx : scroll;
}

float setScrollBy(const SetOverlay& ov, float delta) {
    return setClampScroll(ov, ov.scroll + delta * kSetWheelStep);
}

bool setSlopPassed(float grabV, float v) {
    return fabsf(v - grabV) * kSetHH > kSetDragSlop;
}

float setCloseX() { return kSetHW - kSetPad - kSetCloseR; }
float setTitleY() { return kSetHH - kSetHeadH * 0.5f; }

void setSliderTrack(float* l, float* r) {
    *l = paneL() + 0.34f;
    *r = paneR() - 0.050f;
}

float setSliderValue(float u) {
    float l, r;
    setSliderTrack(&l, &r);
    const float x = u * kSetHW;
    const float v = (x - l) / (r - l);
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

// ------------------------------------------------------------- pickers

int setSegCount(int pick) { return pick == PK_THEME ? 3 : 2; }

void setSegRect(int pick, int opt, float cy, float* x, float* y,
                float* hw, float* hh) {
    const int n = setSegCount(pick);
    *hw = kSetSegCellW * 0.5f - 0.004f;
    *hh = kSetSegHH - 0.004f;
    *x = paneR() - kSetSegCellW * (n - opt) + kSetSegCellW * 0.5f;
    *y = cy;
}

const char* setSegValue(int pick, int opt) {
    if (pick == PK_THEME)
        return opt == 0 ? "dark" : opt == 1 ? "light" : "oled";
    if (pick == PK_LANG)
        return opt == 0 ? "en" : "zh-CN";
    if (pick == PK_DOF)
        return opt == 0 ? "3dof" : "6dof";
    return "";
}

void setScanPill(float textW, float* x, float* hw) {
    const float w = textW * 0.5f + kSetPillPadX;
    *hw = w < kSetPillHW0 ? kSetPillHW0
        : w > kSetPillHW1 ? kSetPillHW1 : w;
    *x = paneR() - kSetPillMR - *hw;
}

float setScanPillL() {
    return paneR() - kSetPillMR - kSetPillHW1 * 2.0f;
}

void setBrandGroup(float rowCx, float minL, float textW, float* iconX,
                   float* textX, float* ty, float* dy) {
    const float w = kSetBrandIconS * 2.0f + kSetBrandGap + textW;
    float l = rowCx - w * 0.5f;
    if (l < minL) l = minL;
    *iconX = l + kSetBrandIconS;
    *textX = l + kSetBrandIconS * 2.0f + kSetBrandGap;
    *ty = 0.019f;
    *dy = -0.030f;
}

// language tag -> option index, the same normalization the flutter app
// applied: English dialects to en, unambiguous Simplified tags to zh-CN
static int langOpt(const std::string& tag) {
    std::string t;
    for (char c : tag) t += (char)tolower((unsigned char)c);
    if (t.rfind("en", 0) == 0) return 0;
    if (t == "zh" || t.rfind("zh-hans", 0) == 0 || t.rfind("zh-cn", 0) == 0 ||
        t.rfind("zh-sg", 0) == 0 || t.rfind("zh-my", 0) == 0) return 1;
    return -1;
}

int setSegCurrent(int pick, const SetSnap& snap) {
    if (pick == PK_THEME) {
        const std::string& t = snap.txt[TX_THEME];
        return t == "light" ? 1 : t == "oled" ? 2 : 0;
    }
    if (pick == PK_LANG) return langOpt(snap.txt[TX_LANG]);
    if (pick == PK_DOF) return (snap.toggles & (1u << TG_DOF6)) ? 1 : 0;
    return -1;
}

bool setImplemented(int rid) {
    switch (rid) {
    case RID_NIGHT:
    case RID_ADB:
    case RID_STAY:
    case RID_TOUCHES:
        return false;
    default:
        return true;
    }
}

bool setRowImplemented(const SetRow& r) {
    switch (r.kind) {
    case RK_NET: case RK_BTROW: case RK_IME: case RK_ENV:
    case RK_CTRL: case RK_XP:
        return true;
    default:
        return setImplemented(r.aux);
    }
}

bool setWifiNeedsPw(const SetWifi& n) {
    return !n.conn && n.saved < 0 && n.sec != WSEC_OPEN;
}

const char* setEnvId(const SetSnap& snap, int aux) {
    if (aux == 0) return "passthrough";
    if (aux == 1) return "builtin";
    const int i = aux - 2;
    if (i < 0 || i >= (int)snap.envs.size()) return "";
    return snap.envs[i].id.c_str();
}

bool setEnvActive(const SetSnap& snap, int aux) {
    const char* sel = snap.txt[TX_ENV].empty() ? "passthrough"
                                               : snap.txt[TX_ENV].c_str();
    return !strcmp(sel, setEnvId(snap, aux));
}

bool setBtNeedsConfirm(const SetBtDev& d) { return d.bonded; }

// ------------------------------------------------------------- dialog

static bool dlgHasKbd(const SetOverlay& ov) {
    return ov.dlg == DLG_WIFI && ov.dlgArg >= 0 &&
           ov.dlgArg < (int)ov.snap.wifi.size() &&
           setWifiNeedsPw(ov.snap.wifi[ov.dlgArg]);
}

void setDlgRect(int dlg, bool kb, float* x, float* y, float* hw,
                float* hh) {
    *x = 0.0f;
    *hw = kSetDlgHW;
    if (dlg == DLG_WIFI && kb) {
        *hh = kSetDlgKbdH * 0.5f;
        *y = -kSetHH + kSetPad + *hh;
    } else {
        *hh = kSetDlgHH;
        *y = 0.10f;
    }
}

// visible button count: wifi carries Forget on saved nets and no Connect
// on the live one; every other dialog is cancel + confirm
int setDlgBtnCount(int dlg, const SetOverlay& ov) {
    if (dlg == DLG_WIFI) {
        if (ov.dlgArg < 0 || ov.dlgArg >= (int)ov.snap.wifi.size())
            return 1;
        const SetWifi& n = ov.snap.wifi[ov.dlgArg];
        return (n.saved >= 0 ? 1 : 0) + 1 + (!n.conn ? 1 : 0);
    }
    return 2;
}

// buttons sit in three fixed slots: 0 left (destructive), 1 mid (cancel),
// 2 right (confirm). A 2-button dialog uses slots 1+2, a lone Cancel the
// middle
void setDlgBtnRect(int dlg, bool kb, int slot, float* x, float* y,
                   float* hw, float* hh) {
    float dx, dy, dhw, dhh;
    setDlgRect(dlg, kb, &dx, &dy, &dhw, &dhh);
    *hw = kSetDlgBtnW * 0.5f;
    *hh = kSetDlgBtnH * 0.5f;
    *y = dy - dhh + kSetDlgPad + *hh;
    if (slot == 0)      *x = dx - dhw + kSetDlgPad + *hw;
    else if (slot == 1) *x = dx + *hw * 0.5f;
    else                *x = dx + dhw - kSetDlgPad - *hw;
}

void setDlgFieldRect(float* x, float* y, float* hw, float* hh) {
    float dx, dy, dhw, dhh;
    setDlgRect(DLG_WIFI, true, &dx, &dy, &dhw, &dhh);
    *x = dx;
    *hw = dhw - kSetDlgPad;
    *hh = kSetDlgFieldH * 0.5f;
    *y = dy + dhh - 0.055f - *hh;
}

// ------------------------------------------------------------- kbd

// key tables: code is a char codepoint or a SK_* negative; label is drawn
// for special keys (chars render themselves)
struct KbdKey { int code; const char* label; float w; };

static const KbdKey kPage0[] = {
    {'1',0,1},{'2',0,1},{'3',0,1},{'4',0,1},{'5',0,1},
    {'6',0,1},{'7',0,1},{'8',0,1},{'9',0,1},{'0',0,1},
    {'q',0,1},{'w',0,1},{'e',0,1},{'r',0,1},{'t',0,1},
    {'y',0,1},{'u',0,1},{'i',0,1},{'o',0,1},{'p',0,1},
    {'a',0,1},{'s',0,1},{'d',0,1},{'f',0,1},{'g',0,1},
    {'h',0,1},{'j',0,1},{'k',0,1},{'l',0,1},{SK_BKSP,"del",1.5f},
    {SK_SHIFT,"shift",1.5f},{'z',0,1},{'x',0,1},{'c',0,1},{'v',0,1},
    {'b',0,1},{'n',0,1},{'m',0,1},{'.',0,1},{SK_DONE,"done",1.5f},
    {SK_PAGE,"?123",2.0f},{' ',"space",8.0f},
};

static const KbdKey kPage1[] = {
    {'!',0,1},{'@',0,1},{'#',0,1},{'$',0,1},{'%',0,1},
    {'^',0,1},{'&',0,1},{'*',0,1},{'(',0,1},{')',0,1},
    {'-',0,1},{'_',0,1},{'=',0,1},{'+',0,1},{'[',0,1},
    {']',0,1},{'{',0,1},{'}',0,1},{'\\',0,1},{'|',0,1},
    {';',0,1},{':',0,1},{'\'',0,1},{'"',0,1},{'<',0,1},
    {'>',0,1},{',',0,1},{'.',0,1},{'?',0,1},{SK_BKSP,"del",1.5f},
    {'/',0,1},{'~',0,1},{'`',0,1},{SK_PAGE,"abc",1.5f},{SK_DONE,"done",1.5f},
    {SK_PAGE,"abc",2.0f},{' ',"space",8.0f},
};

// row sizes per page
static const int kPage0Rows[] = {10, 10, 10, 11, 2};
static const int kPage1Rows[] = {10, 10, 10, 5, 2};
static const int kPageN[] = {
    (int)(sizeof(kPage0) / sizeof(kPage0[0])),
    (int)(sizeof(kPage1) / sizeof(kPage1[0])),
};
static const KbdKey* kPages[] = {kPage0, kPage1};
static const int* kPageRows[] = {kPage0Rows, kPage1Rows};

int setKbdCount(int page) {
    return (page >= 0 && page <= 1) ? kPageN[page] : 0;
}

void setKeyRect(int page, int key, float* x, float* y, float* hw,
                float* hh) {
    *x = *y = *hw = *hh = 0.0f;
    if (page < 0 || page > 1 || key < 0 || key >= kPageN[page]) return;
    const KbdKey* keys = kPages[page];
    const int* rows = kPageRows[page];
    int row = 0, col = 0;
    for (int i = 0; i < key; ++i)
        if (++col >= rows[row]) { col = 0; ++row; }
    float units = 0.0f, lead = 0.0f;
    int base = 0;
    for (int r = 0; r < row; ++r) base += rows[r];
    for (int i = 0; i < rows[row]; ++i) {
        units += keys[base + i].w;
        if (i < col) lead += keys[base + i].w;
    }
    float dx, dy, dhw, dhh;
    setDlgRect(DLG_WIFI, true, &dx, &dy, &dhw, &dhh);
    const float khw = dhw - kSetDlgPad;
    const float uw = khw * 2.0f / units;
    const float rh = kSetKeyH;
    // keyboard packs under the password field, above the button row
    float fx, fy, fhw, fhh;
    setDlgFieldRect(&fx, &fy, &fhw, &fhh);
    const float top = fy - fhh - 0.016f;
    *y = top - rh * (row + 0.5f);
    *hh = rh * 0.5f - 0.005f;
    *x = dx - khw + (lead + keys[key].w * 0.5f) * uw;
    *hw = keys[key].w * uw * 0.5f - 0.004f;
}

int setKeyCode(int page, int key, bool shift) {
    if (page < 0 || page > 1 || key < 0 || key >= kPageN[page]) return 0;
    int c = kPages[page][key].code;
    if (shift && page == 0 && c >= 'a' && c <= 'z') c += 'A' - 'a';
    return c;
}

const char* setKeyLabel(int page, int key) {
    if (page < 0 || page > 1 || key < 0 || key >= kPageN[page]) return "";
    const char* l = kPages[page][key].label;
    return l ? l : "";
}

// ------------------------------------------------------------- zones

// content row at a card-local point (pre-scroll v input), or -1. Only
// hits inside the clip band and the pane count
static int rowAt(const SetOverlay& ov, float u, float v) {
    const float x = u * kSetHW;
    float lo, hi;
    setClipBand(&lo, &hi);
    const float y = v * kSetHH;
    if (x < paneL() || x > paneR() || y < lo || y > hi) return -1;
    const float sy = y - ov.scroll;
    for (int i = 0; i < (int)ov.rows.size(); ++i) {
        const float top = rowTop(ov, i);
        const float h = rowH(ov.rows[i].kind);
        if (sy <= top && sy >= top - h) return i;
    }
    return -1;
}

static int secAt(const SetOverlay& ov, float u, float v) {
    const float x = u * kSetHW, y = v * kSetHH;
    if (x < setSideL() || x > setSideR()) return -1;
    const int n = setSecCount(ov.android);
    for (int i = 0; i < n; ++i) {
        float sx, sy, hw, hh;
        setSecRect(i, &sx, &sy, &hw, &hh);
        if (x >= sx - hw && x <= sx + hw && y >= sy - hh && y <= sy + hh)
            return setSecAt(ov.android, i);
    }
    return -1;
}

static int dlgZoneAt(const SetOverlay& ov, float u, float v,
                     int* idx, int*) {
    const float x = u * kSetHW, y = v * kSetHH;
    float dx, dy, dhw, dhh;
    const bool kb = dlgHasKbd(ov);
    setDlgRect(ov.dlg, kb, &dx, &dy, &dhw, &dhh);
    if (x < dx - dhw || x > dx + dhw || y < dy - dhh || y > dy + dhh)
        return SZONE_BODY;   // modal: outside hits die on the card
    if (kb) {
        for (int i = 0; i < setKbdCount(ov.kbdPage); ++i) {
            float kx, ky, khw, khh;
            setKeyRect(ov.kbdPage, i, &kx, &ky, &khw, &khh);
            if (x >= kx - khw && x <= kx + khw &&
                y >= ky - khh && y <= ky + khh) {
                *idx = i;
                return SZONE_KEY;
            }
        }
    }
    const int nb = setDlgBtnCount(ov.dlg, ov);
    for (int i = 0; i < nb; ++i) {
        // visible order left->right: forget(0), cancel, confirm. With no
        // forget the row starts at slot 1
        const int slot = nb == 3 ? i : i + (nb == 2 ? 1 : 1);
        float bx, by, bhw, bhh;
        setDlgBtnRect(ov.dlg, kb, slot, &bx, &by, &bhw, &bhh);
        if (x >= bx - bhw && x <= bx + bhw && y >= by - bhh &&
            y <= by + bhh) {
            *idx = i;
            return SZONE_BTN;
        }
    }
    return SZONE_BODY;
}

// a content row's inner zones: the slider track, segmented cells, the
// env row's delete disc, the scan card's refresh pill
static int rowZone(const SetOverlay& ov, int i, float u, float v,
                   int* aux) {
    const SetRow& r = ov.rows[i];
    float rx, ry, hw, hh;
    setRowRect(ov, i, &rx, &ry, &hw, &hh);
    const float x = u * kSetHW, y = v * kSetHH;
    switch (r.kind) {
    case RK_SLIDER: {
        float l, rr;
        setSliderTrack(&l, &rr);
        return x >= l - 0.02f ? SZONE_SLIDER : SZONE_ROW;
    }
    case RK_SEG: {
        const int n = setSegCount(r.id);
        for (int o = 0; o < n; ++o) {
            float sx, sy, shw, shh;
            setSegRect(r.id, o, ry, &sx, &sy, &shw, &shh);
            if (x >= sx - shw && x <= sx + shw &&
                y >= sy - shh && y <= sy + shh) {
                *aux = o;
                return SZONE_SEG;
            }
        }
        return SZONE_ROW;
    }
    case RK_SCANHDR:
        // ctrl's card is the scan button itself; wifi/bt/env/ime carry a
        // refresh pill on the right edge (ime's is inert - the platform
        // list needs no scan)
        if (r.id == SEC_CTRL) return SZONE_ROW;
        if (x > setScanPillL()) return SZONE_SCAN;
        return SZONE_ROW;
    case RK_ENV:
        if (r.aux >= 2 && x > paneR() - 0.10f) return SZONE_DEL;
        return SZONE_ROW;
    default:
        return SZONE_ROW;
    }
}

int setZoneAt(const SetOverlay& ov, float u, float v, int* idx, int* aux) {
    *idx = -1;
    *aux = -1;
    if (fabsf(u) > 1.0f || fabsf(v) > 1.0f) return SZONE_NONE;
    if (ov.dlg != DLG_NONE) {
        const int z = dlgZoneAt(ov, u, v, idx, aux);
        return z == SZONE_NONE ? SZONE_BODY : z;
    }
    const float x = u * kSetHW, y = v * kSetHH;
    if (y > kSetHH - kSetHeadH) {
        const float dx = x - setCloseX(), dy = y - setTitleY();
        const float rr = kSetCloseR + 0.012f;
        if (dx * dx + dy * dy <= rr * rr) return SZONE_CLOSE;
        return SZONE_BODY;
    }
    const int s = secAt(ov, u, v);
    if (s >= 0) { *idx = s; return SZONE_SEC; }
    const int i = rowAt(ov, u, v);
    if (i >= 0) { *idx = i; return rowZone(ov, i, u, v, aux); }
    return SZONE_BODY;
}

SetHit pickSetRay(const SetOverlay& ov, float yaw, float pitch,
                  const float origin[3], const float o[3],
                  const float d[3]) {
    SetHit pk;
    float c[3], r[3], up[3];
    setCenter(yaw, pitch, origin, c, r, up);
    if (!rayQuad(c, r, up, origin, o, d, kSetHW, kSetHH,
                 &pk.u, &pk.v, &pk.t)) return pk;
    if (fabsf(pk.u) > 1.0f || fabsf(pk.v) > 1.0f) return pk;
    pk.hit = true;
    pk.zone = setZoneAt(ov, pk.u, pk.v, &pk.idx, &pk.aux);
    return pk;
}
